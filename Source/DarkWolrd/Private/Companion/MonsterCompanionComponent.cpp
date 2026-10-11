// Fill out your copyright notice in the Description page of Project Settings.

#include "Companion/MonsterCompanionComponent.h"
#include "Monster/BaseMonster.h"
#include "Summon/SummonAIComponent.h"
#include "Stats/CharacterStatComponent.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMonsterCompanionComponent::UMonsterCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 알림에 쓸 종족 이름
static FText GetMonsterClassName(TSubclassOf<ABaseMonster> MonsterClass)
{
	const ABaseMonster* DefaultMonster = MonsterClass ? MonsterClass->GetDefaultObject<ABaseMonster>() : nullptr;
	return DefaultMonster ? DefaultMonster->GetMonsterDisplayName() : FText::GetEmpty();
}


// 카드 습득: 보유 목록에 추가. bAutoRegisterCards면 바로 등록(소환 해금/강화)까지 함
void UMonsterCompanionComponent::AddCard(const FMonsterCardEntry& CardData)
{
	HeldCards.Add(CardData);

	if (bAutoRegisterCards)
	{
		RegisterCard(HeldCards.Num() - 1);
	}
	else
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(
			NSLOCTEXT("Summon", "CardPickedUp", "{0} 카드 획득"), GetMonsterClassName(CardData.MonsterClass)), 3.f);
	}
}


// 카드 등록: 신규 종족이면 도감에 새로 추가, 이미 등록된 종족이면 업그레이드 카운트만 증가(최대치 제한)
bool UMonsterCompanionComponent::RegisterCard(int32 HeldCardIndex)
{
	if (!HeldCards.IsValidIndex(HeldCardIndex)) return false;

	const FMonsterCardEntry CardData = HeldCards[HeldCardIndex];
	HeldCards.RemoveAt(HeldCardIndex);

	const FText MonsterName = GetMonsterClassName(CardData.MonsterClass);
	FText Message;

	if (FRegisteredMonsterEntry* Existing = FindRegisteredEntry(CardData.MonsterClass))
	{
		if (Existing->UpgradeCount < MaxUpgradeCount)
		{
			Existing->UpgradeCount++;
			Message = FText::Format(NSLOCTEXT("Summon", "CardUpgraded", "{0} 강화 +{1}"), MonsterName, Existing->UpgradeCount);
		}
		else
		{
			Message = FText::Format(NSLOCTEXT("Summon", "CardMaxed", "{0}은(는) 이미 최대 강화 상태입니다"), MonsterName);
		}
	}
	else
	{
		FRegisteredMonsterEntry NewEntry;
		NewEntry.MonsterClass = CardData.MonsterClass;
		NewEntry.UpgradeCount = 0;
		RegisteredMonsters.Add(NewEntry);
		Message = FText::Format(NSLOCTEXT("Summon", "CardRegistered", "{0} 도감 등록! 소환 키로 불러낼 수 있습니다"), MonsterName);
	}

	UGameAnnouncementSubsystem::AnnounceInWorld(this, Message, 3.f);
	return true;
}


// 보유 카드 전부 등록: 항상 맨 앞 카드를 등록하면 목록이 앞으로 당겨지므로 빌 때까지 0번을 반복
int32 UMonsterCompanionComponent::RegisterAllHeldCards()
{
	int32 RegisteredCount = 0;
	while (HeldCards.Num() > 0 && RegisterCard(0))
	{
		++RegisteredCount;
	}
	return RegisteredCount;
}


// 등록된 도감에서 해당 종족 항목 검색
FRegisteredMonsterEntry* UMonsterCompanionComponent::FindRegisteredEntry(TSubclassOf<ABaseMonster> MonsterClass)
{
	return RegisteredMonsters.FindByPredicate([MonsterClass](const FRegisteredMonsterEntry& Entry)
	{
		return Entry.MonsterClass == MonsterClass;
	});
}


// Y 입력 처리: 기존 소환수를 전부 취소하고 곧바로 HP가 가득 찬 새 개체로 재소환.
// 패시브는 마지막에 한 번만 다시 계산함 (중간에 해제 상태로 계산하면 늘어났던 최대 HP가 줄면서 현재 HP가 깎임)
void UMonsterCompanionComponent::Resummon()
{
	if (bSummonLocked)
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Summon", "SummonLocked", "지금은 재소환할 수 없습니다"), 2.f);
		return;
	}
	if (GetResummonCooldownRemaining() > 0.f) return;

	DestroyActiveSummons();
	SpawnSummons();
	RefreshPartnerPassives();

	// 소환할 종족이 없어 아무것도 안 나왔으면 쿨타임을 걸지 않고 안내만 함
	if (ActiveSummons.Num() > 0)
	{
		LastResummonTime = GetWorld()->GetTimeSeconds();
	}
	else
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this,
			NSLOCTEXT("Summon", "NothingToSummon", "소환할 몬스터가 없습니다. 몬스터를 쓰러뜨려 카드를 얻으세요"), 3.f);
	}
}


float UMonsterCompanionComponent::GetResummonCooldownRemaining() const
{
	if (ResummonCooldown <= 0.f || !GetWorld()) return 0.f;

	return FMath::Max(0.f, LastResummonTime + ResummonCooldown - GetWorld()->GetTimeSeconds());
}


// 소환 해제: 소환수를 전부 없애고 파트너 패시브도 해제
void UMonsterCompanionComponent::DespawnAllSummons()
{
	DestroyActiveSummons();
	RefreshPartnerPassives();
}


// 액터를 Destroy하면 그 안의 SummonAIComponent와 모든 우선순위 정보가 함께 사라짐
void UMonsterCompanionComponent::DestroyActiveSummons()
{
	for (ABaseMonster* Summon : ActiveSummons)
	{
		if (IsValid(Summon))
		{
			// 사라지는 연출 (아직 나타나기 전이라 숨겨진 소환수는 생략)
			if (!Summon->IsHidden())
			{
				if (DespawnVFX)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DespawnVFX, Summon->GetActorLocation());
				}
				if (DespawnSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, DespawnSound, Summon->GetActorLocation());
				}
			}
			Summon->Destroy();
		}
	}
	ActiveSummons.Empty();
}


// 소환수 사망: 목록에서 빼고 패시브 재계산 (액터 파괴는 BaseMonster::Die가 직접 함)
void UMonsterCompanionComponent::HandleSummonDied(ABaseMonster* Summon)
{
	ActiveSummons.Remove(Summon);
	RefreshPartnerPassives();
}


// 파트너 패시브 재계산: 전부 지우고 살아있는 소환수의 패시브를 카드 등급 배율을 곱해서 다시 더함.
// 최대 HP/MP가 줄었으면 현재 값도 새 최대치에 맞춰 깎음
void UMonsterCompanionComponent::RefreshPartnerPassives()
{
	UCharacterStatComponent* StatComponent = GetOwner() ? GetOwner()->FindComponentByClass<UCharacterStatComponent>() : nullptr;
	if (!StatComponent) return;

	StatComponent->ClearSummonPassives();

	for (ABaseMonster* Summon : ActiveSummons)
	{
		if (!IsValid(Summon) || !Summon->IsAlive() || Summon->PartnerPassiveType == ESummonPassiveType::None) continue;

		const FRegisteredMonsterEntry* Entry = FindRegisteredEntry(Summon->GetClass());
		const float GradeMultiplier = Entry ? Entry->GetUpgradeMultiplier() : 1.f;
		StatComponent->AddSummonPassive(Summon->PartnerPassiveType, Summon->PartnerPassiveValue * GradeMultiplier);
	}

	StatComponent->CurrentHP = FMath::Min(StatComponent->CurrentHP, StatComponent->GetMaxHP());
	StatComponent->CurrentMP = FMath::Min(StatComponent->CurrentMP, StatComponent->GetMaxMP());
}


// 도감에서 소환할 종족을 골라 플레이어 주변에 스폰하고 소환수 상태로 활성화
void UMonsterCompanionComponent::SpawnSummons()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<TSubclassOf<ABaseMonster>> ClassesToSummon = SelectedSummonSlots;
	if (ClassesToSummon.Num() == 0)
	{
		for (const FRegisteredMonsterEntry& Entry : RegisteredMonsters)
		{
			if (Entry.MonsterClass)
			{
				ClassesToSummon.Add(Entry.MonsterClass);
			}
		}
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	int32 SpawnedCount = 0;

	for (TSubclassOf<ABaseMonster> MonsterClass : ClassesToSummon)
	{
		if (SpawnedCount >= MaxActiveSummons) break;
		if (!MonsterClass) continue;

		FRegisteredMonsterEntry* Entry = FindRegisteredEntry(MonsterClass);
		if (!Entry) continue; // 등록되지 않은 종족은 소환 불가

		// 플레이어 주변 원형으로 흩어서 스폰 (겹침 방지)
		const float Angle = FMath::DegreesToRadians(120.f * SpawnedCount);
		const FVector SpawnOffset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 200.f;
		const FVector SpawnLocation = OwnerLocation + SpawnOffset;

		// 스폰을 잠시 미뤄두고(Deferred) 소환수 정보를 먼저 채운 뒤 마무리함.
		// 이렇게 해야 BeginPlay 시점에 이미 소환수라서 야생 AI가 켜지지 않고, HP도 레벨·등급이 반영된 값으로 채워짐
		const FTransform SpawnTransform(OwnerActor->GetActorRotation(), SpawnLocation);
		ABaseMonster* NewSummon = World->SpawnActorDeferred<ABaseMonster>(MonsterClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!NewSummon) continue;

		NewSummon->bIsPlayerSummon = true;
		NewSummon->Faction = ECombatFaction::Player;
		NewSummon->Level = Entry->Level;
		NewSummon->SummonPowerMultiplier = Entry->GetUpgradeMultiplier();
		NewSummon->FinishSpawning(SpawnTransform);

		// 몬스터 BP에서 AutoPossessAI를 바꿔뒀어도 AI 컨트롤러가 꼭 붙도록 함
		if (!NewSummon->GetController())
		{
			NewSummon->SpawnDefaultController();
		}
		NewSummon->CurrentHP = NewSummon->GetMaxHP();

		if (NewSummon->SummonAIComponent)
		{
			NewSummon->SummonAIComponent->ActivateSummonAI(OwnerActor, SpawnedCount);
		}
		ApplyAutoHuntSettings(NewSummon);

		ActiveSummons.Add(NewSummon);

		// 등장 연출: 첫 마리는 바로, 나머지는 SummonStagger 간격으로 한 마리씩 나타남 (나타나기 전까지는 숨겨둠)
		const float RevealDelay = SpawnedCount * SummonStagger;
		if (RevealDelay <= 0.f)
		{
			RevealSummon(NewSummon);
		}
		else
		{
			NewSummon->SetActorHiddenInGame(true);
			TWeakObjectPtr<UMonsterCompanionComponent> WeakThis(this);
			FTimerHandle RevealTimerHandle;
			World->GetTimerManager().SetTimer(RevealTimerHandle, FTimerDelegate::CreateWeakLambda(NewSummon, [WeakThis, NewSummon]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->RevealSummon(NewSummon);
				}
				else
				{
					NewSummon->SetActorHiddenInGame(false);
				}
			}), RevealDelay, false);
		}

		++SpawnedCount;
	}

	// 플레이어 시전 동작
	if (SpawnedCount > 0 && SummonCastMontage)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
		{
			OwnerCharacter->PlayAnimMontage(SummonCastMontage);
		}
	}
}


// 등장 연출: 보이게 한 뒤 발밑에서 이펙트·사운드를 재생하고, 몸이 튀어나오듯 커지게 함
void UMonsterCompanionComponent::RevealSummon(ABaseMonster* Summon)
{
	if (!IsValid(Summon)) return;

	Summon->SetActorHiddenInGame(false);

	const float HalfHeight = Summon->GetCapsuleComponent() ? Summon->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	const FVector FeetLocation = Summon->GetActorLocation() - FVector(0.f, 0.f, HalfHeight);

	UNiagaraSystem* VFX = Summon->SummonVFX ? Summon->SummonVFX : SummonVFX;
	USoundBase* Sound = Summon->SummonSound ? Summon->SummonSound : SummonSound;
	if (VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, VFX, FeetLocation);
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, FeetLocation);
	}

	Summon->PlaySummonAppear(SummonAppearDuration);
}


// 경험치 지급 + 레벨업 처리. 해당 종족이 지금 소환되어 있으면 살아있는 개체 스탯도 즉시 갱신
void UMonsterCompanionComponent::AddSummonEXP(TSubclassOf<ABaseMonster> MonsterClass, float EXP)
{
	FRegisteredMonsterEntry* Entry = FindRegisteredEntry(MonsterClass);
	if (!Entry) return;

	Entry->CurrentEXP += FMath::RoundToInt(EXP);

	while (Entry->CurrentEXP >= Entry->GetEXPToNextLevel())
	{
		Entry->CurrentEXP -= Entry->GetEXPToNextLevel();
		Entry->Level += 1;
	}

	for (ABaseMonster* Summon : ActiveSummons)
	{
		if (IsValid(Summon) && Summon->GetClass() == MonsterClass)
		{
			Summon->Level = Entry->Level;
			const float NewMaxHP = Summon->GetMaxHP();
			Summon->CurrentHP = FMath::Min(Summon->CurrentHP, NewMaxHP);
		}
	}
}


// "4+1": 모든 소환수를 플레이어 주변에 강제 집결시킴
void UMonsterCompanionComponent::HandleGatherCommand()
{
	for (ABaseMonster* Summon : ActiveSummons)
	{
		if (IsValid(Summon) && Summon->SummonAIComponent)
		{
			Summon->SummonAIComponent->CommandGather();
		}
	}
}


// "4+2": 모든 소환수가 플레이어의 현재 락온 대상을 향해 싸우게 함
void UMonsterCompanionComponent::HandleAttackPlayerTargetCommand()
{
	for (ABaseMonster* Summon : ActiveSummons)
	{
		if (IsValid(Summon) && Summon->SummonAIComponent)
		{
			Summon->SummonAIComponent->CommandAttackPlayerTarget();
		}
	}
}


// 협공 대상 기록: 플레이어가 때린 적과 플레이어를 때린 적 중 가장 최근 것을 기억
void UMonsterCompanionComponent::NotifyPlayerHitEnemy(AActor* Enemy)
{
	if (!CombatFaction::AreHostile(GetOwner(), Enemy)) return;

	AssistTarget = Enemy;
	AssistTargetTime = GetWorld()->GetTimeSeconds();
}


void UMonsterCompanionComponent::NotifyPlayerAttacked(AActor* Attacker)
{
	NotifyPlayerHitEnemy(Attacker);
}


AActor* UMonsterCompanionComponent::GetAssistTarget() const
{
	AActor* Target = AssistTarget.Get();
	if (!Target || !GetOwner()) return nullptr;
	if (GetWorld()->GetTimeSeconds() - AssistTargetTime > AssistMemoryTime) return nullptr;
	if (!CombatFaction::IsAlive(Target) || !CombatFaction::AreHostile(GetOwner(), Target)) return nullptr;
	if (FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation()) > AssistRadius) return nullptr;

	return Target;
}


// 자동 사냥 켜기/끄기: 설정을 바꾸고 지금 소환되어 있는 소환수에도 바로 적용
void UMonsterCompanionComponent::SetSummonAutoHunt(bool bEnabled)
{
	bSummonAutoHunt = bEnabled;
	for (ABaseMonster* Summon : ActiveSummons)
	{
		ApplyAutoHuntSettings(Summon);
	}
}


// 자동 사냥 설정 적용: 소환수가 스스로 적을 찾되, 플레이어 곁(AutoHuntRadius)을 벗어나 사냥하지 않게 함
void UMonsterCompanionComponent::ApplyAutoHuntSettings(ABaseMonster* Summon) const
{
	if (!IsValid(Summon) || !Summon->SummonAIComponent) return;

	Summon->SummonAIComponent->bAutoEngage = bSummonAutoHunt;
	Summon->SummonAIComponent->AutoEngageLeashRadius = AutoHuntRadius;
}
