// Fill out your copyright notice in the Description page of Project Settings.

#include "Companion/MonsterCompanionComponent.h"
#include "Monster/BaseMonster.h"
#include "Summon/SummonAIComponent.h"
#include "Stats/CharacterStatComponent.h"

UMonsterCompanionComponent::UMonsterCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 카드 습득: 보유 목록에 추가만 하고, 등록(소환 해금)은 별도로 RegisterCard를 호출해야 함
void UMonsterCompanionComponent::AddCard(const FMonsterCardEntry& CardData)
{
	HeldCards.Add(CardData);
}


// 카드 등록: 신규 종족이면 도감에 새로 추가, 이미 등록된 종족이면 업그레이드 카운트만 증가(최대치 제한)
bool UMonsterCompanionComponent::RegisterCard(int32 HeldCardIndex)
{
	if (!HeldCards.IsValidIndex(HeldCardIndex)) return false;

	const FMonsterCardEntry CardData = HeldCards[HeldCardIndex];
	HeldCards.RemoveAt(HeldCardIndex);

	if (FRegisteredMonsterEntry* Existing = FindRegisteredEntry(CardData.MonsterClass))
	{
		Existing->UpgradeCount = FMath::Min(Existing->UpgradeCount + 1, MaxUpgradeCount);
	}
	else
	{
		FRegisteredMonsterEntry NewEntry;
		NewEntry.MonsterClass = CardData.MonsterClass;
		NewEntry.UpgradeCount = 0;
		RegisteredMonsters.Add(NewEntry);
	}

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
	if (GetResummonCooldownRemaining() > 0.f) return;

	DestroyActiveSummons();
	SpawnSummons();
	RefreshPartnerPassives();

	// 소환할 종족이 없어 아무것도 안 나왔으면 쿨타임을 걸지 않음
	if (ActiveSummons.Num() > 0)
	{
		LastResummonTime = GetWorld()->GetTimeSeconds();
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

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ABaseMonster* NewSummon = World->SpawnActor<ABaseMonster>(MonsterClass, SpawnLocation, OwnerActor->GetActorRotation(), SpawnParams);
		if (!NewSummon) continue;

		// 코드로 스폰한 Pawn은 AutoPossessAI 기본값(PlacedInWorld) 때문에 AI 컨트롤러가 안 붙음 → 직접 붙여줌
		if (!NewSummon->GetController())
		{
			NewSummon->SpawnDefaultController();
		}

		NewSummon->bIsPlayerSummon = true;
		NewSummon->Level = Entry->Level;
		NewSummon->SummonPowerMultiplier = Entry->GetUpgradeMultiplier();
		NewSummon->CurrentHP = NewSummon->GetMaxHP();

		if (NewSummon->SummonAIComponent)
		{
			NewSummon->SummonAIComponent->ActivateSummonAI(OwnerActor, SpawnedCount);
		}

		ActiveSummons.Add(NewSummon);
		++SpawnedCount;
	}
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
