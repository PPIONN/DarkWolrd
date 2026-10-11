// Fill out your copyright notice in the Description page of Project Settings.

#include "Arena/MonsterArena.h"
#include "Monster/BaseMonster.h"
#include "AI/WildMonsterAIComponent.h"
#include "Summon/SummonAIComponent.h"
#include "Character/Rabbit.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Stats/CharacterStatComponent.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "TimerManager.h"

AMonsterArena::AMonsterArena()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;

	ChallengeTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ChallengeTrigger"));
	ChallengeTrigger->SetupAttachment(RootComponent);
	ChallengeTrigger->SetSphereRadius(120.f);
	ChallengeTrigger->SetRelativeLocation(FVector(-1500.f, 0.f, 0.f));
	ChallengeTrigger->SetCollisionProfileName(TEXT("Trigger"));

	StartAnnouncement = NSLOCTEXT("Arena", "Start", "몬스터대전 {Rank}단계 - 상대: {Opponent}");
	WinAnnouncement = NSLOCTEXT("Arena", "Win", "승리! {Opponent}을(를) 이겼습니다 {Reason}  {Reward}");
	LoseAnnouncement = NSLOCTEXT("Arena", "Lose", "패배... {Opponent}에게 졌습니다 {Reason}");
}


void AMonsterArena::BeginPlay()
{
	Super::BeginPlay();

	ChallengeTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMonsterArena::HandleTriggerBeginOverlap);
}


void AMonsterArena::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ARabbit* Rabbit = Cast<ARabbit>(OtherActor))
	{
		StartMatch(Rabbit);
	}
}


const FArenaOpponent* AMonsterArena::GetCurrentOpponent() const
{
	if (Ladder.Num() == 0) return nullptr;
	return &Ladder[FMath::Clamp(CurrentRank, 0, Ladder.Num() - 1)];
}


FText AMonsterArena::GetCurrentOpponentName() const
{
	const FArenaOpponent* Opponent = GetCurrentOpponent();
	return Opponent ? Opponent->OpponentName : FText::GetEmpty();
}


float AMonsterArena::GetTimeRemaining() const
{
	if (ArenaState != EArenaState::Fighting) return 0.f;
	return FMath::Max(0.f, TimeLimit - (GetWorld()->GetTimeSeconds() - FightStartTime));
}


// 시작: 대기 상태이고, 상대가 정해져 있고, 살아있는 소환수가 있어야 함. 상대를 세워두고 카운트다운
bool AMonsterArena::StartMatch(ARabbit* InChallenger)
{
	if (ArenaState != EArenaState::Idle || !InChallenger || InChallenger->IsDead()) return false;
	if (GetWorld()->GetTimeSeconds() < NextAllowedStartTime) return false;

	const FArenaOpponent* Opponent = GetCurrentOpponent();
	if (!Opponent || Opponent->Monsters.Num() == 0)
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Arena", "NoOpponent", "아레나에 상대(Ladder)가 설정되지 않았습니다"), 3.f);
		return false;
	}

	const UMonsterCompanionComponent* Companion = InChallenger->FindComponentByClass<UMonsterCompanionComponent>();
	const bool bHasSummon = Companion && Companion->ActiveSummons.ContainsByPredicate(
		[](const ABaseMonster* Summon) { return IsValid(Summon) && Summon->IsAlive(); });
	if (!bHasSummon)
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Arena", "NeedSummons", "소환수를 먼저 소환하세요 (Y)"), 3.f);
		return false;
	}

	Challenger = InChallenger;
	ArenaState = EArenaState::Countdown;
	SpawnOpponents();

	FFormatNamedArguments Args;
	Args.Add(TEXT("Rank"), CurrentRank + 1);
	Args.Add(TEXT("Opponent"), Opponent->OpponentName);
	UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(StartAnnouncement, Args), 2.f);

	CountdownRemaining = CountdownSeconds;
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &AMonsterArena::CountdownTick, 1.f, true, 2.f);
	return true;
}


// 상대 소환: AI는 꺼둔 채로 세워두고(카운트다운 동안 대기), 소환수끼리만 싸우도록 플레이어 캐릭터는 무시하게 설정
void AMonsterArena::SpawnOpponents()
{
	OpponentTeam.Reset();

	const FArenaOpponent* Opponent = GetCurrentOpponent();
	if (!Opponent) return;

	const int32 Count = Opponent->Monsters.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FArenaOpponentMonster& Entry = Opponent->Monsters[Index];
		if (!Entry.MonsterClass) continue;

		const ABaseMonster* Defaults = Entry.MonsterClass->GetDefaultObject<ABaseMonster>();
		const float HalfHeight = Defaults ? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
		const FVector Ground = GetSpotLocation(true, Index, Count);
		const FRotator FacingCenter(0.f, (GetActorLocation() - Ground).Rotation().Yaw, 0.f);
		const FTransform SpawnTransform(FacingCenter, Ground + FVector(0.f, 0.f, HalfHeight));

		ABaseMonster* Monster = GetWorld()->SpawnActorDeferred<ABaseMonster>(Entry.MonsterClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Monster) continue;

		Monster->Faction = ECombatFaction::ArenaOpponent;
		Monster->bIgnoresPlayerCharacter = true;
		Monster->Level = Entry.Level;
		Monster->SummonPowerMultiplier = 1.f + Entry.UpgradeCount * 0.15f;
		Monster->CardDropChance = 0.f;
		if (Monster->WildAIComponent)
		{
			Monster->WildAIComponent->bAutoStart = false;
			Monster->WildAIComponent->bAggressive = true;
			Monster->WildAIComponent->AggroRadius = ArenaRadius * 3.f;
			Monster->WildAIComponent->LeashRadius = 100000.f;
			Monster->WildAIComponent->bHealOnReturn = false;
			Monster->WildAIComponent->bPatrol = false;
		}
		Monster->FinishSpawning(SpawnTransform);

		Monster->OnMonsterDied.AddDynamic(this, &AMonsterArena::HandleCombatantDied);
		OpponentTeam.Add(Monster);
	}
}


void AMonsterArena::CountdownTick()
{
	if (ArenaState != EArenaState::Countdown) return;

	if (CountdownRemaining > 0)
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::AsNumber(CountdownRemaining), 1.f);
		--CountdownRemaining;
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	BeginFight();
}


// 대전 시작: 지금 살아있는 소환수로 팀을 꾸려 내 쪽 자리에 세우고 HP를 채운 뒤, 양쪽 AI를 켬
void AMonsterArena::BeginFight()
{
	ARabbit* Rabbit = Challenger.Get();
	UMonsterCompanionComponent* Companion = Rabbit ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
	if (!Companion)
	{
		EndMatch(false, NSLOCTEXT("Arena", "ReasonNoPlayer", "(도전자 없음)"));
		return;
	}

	PlayerTeam.Reset();
	for (ABaseMonster* Summon : Companion->ActiveSummons)
	{
		if (IsValid(Summon) && Summon->IsAlive())
		{
			PlayerTeam.Add(Summon);
		}
	}
	if (PlayerTeam.Num() == 0)
	{
		EndMatch(false, NSLOCTEXT("Arena", "ReasonNoSummon", "(소환수 없음)"));
		return;
	}

	for (int32 Index = 0; Index < PlayerTeam.Num(); ++Index)
	{
		ABaseMonster* Summon = PlayerTeam[Index].Get();
		const float HalfHeight = Summon->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Ground = GetSpotLocation(false, Index, PlayerTeam.Num());
		const FRotator FacingCenter(0.f, (GetActorLocation() - Ground).Rotation().Yaw, 0.f);

		if (AAIController* AICon = Cast<AAIController>(Summon->GetController()))
		{
			AICon->StopMovement();
		}
		Summon->TeleportTo(Ground + FVector(0.f, 0.f, HalfHeight), FacingCenter);
		Summon->CurrentHP = Summon->GetMaxHP();
		if (Summon->SummonAIComponent)
		{
			// 아레나 안에서는 플레이어 위치와 상관없이 상대를 찾아 싸움
			Summon->SummonAIComponent->bAutoEngage = true;
			Summon->SummonAIComponent->AutoEngageLeashRadius = 0.f;
		}
		Summon->OnMonsterDied.AddDynamic(this, &AMonsterArena::HandleCombatantDied);
	}

	for (const TWeakObjectPtr<ABaseMonster>& Weak : OpponentTeam)
	{
		if (ABaseMonster* Monster = Weak.Get())
		{
			if (Monster->WildAIComponent)
			{
				Monster->WildAIComponent->StartWildAI();
			}
		}
	}

	Rabbit->SetCanBeDamaged(false);
	Companion->SetSummonLocked(true);

	ArenaState = EArenaState::Fighting;
	FightStartTime = GetWorld()->GetTimeSeconds();
	UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Arena", "Go", "시작!"), 1.5f);
	OnMatchStarted.Broadcast();

	GetWorldTimerManager().SetTimer(CheckTimerHandle, this, &AMonsterArena::CheckMatchState, 0.5f, true);
}


void AMonsterArena::HandleCombatantDied(ABaseMonster* DeadMonster)
{
	if (ArenaState == EArenaState::Fighting)
	{
		CheckMatchState();
	}
}


// 판정: 전멸 → 승패, 플레이어 이탈 → 기권, 제한 시간 → 남은 HP 비율 비교 (같으면 패배)
void AMonsterArena::CheckMatchState()
{
	if (ArenaState != EArenaState::Fighting) return;

	ARabbit* Rabbit = Challenger.Get();
	if (!Rabbit)
	{
		EndMatch(false, NSLOCTEXT("Arena", "ReasonNoPlayer", "(도전자 없음)"));
		return;
	}

	if (CountAlive(OpponentTeam) == 0)
	{
		EndMatch(true, FText::GetEmpty());
		return;
	}
	if (CountAlive(PlayerTeam) == 0)
	{
		EndMatch(false, FText::GetEmpty());
		return;
	}
	if (FVector::Dist2D(Rabbit->GetActorLocation(), GetActorLocation()) > LeaveDistance)
	{
		EndMatch(false, NSLOCTEXT("Arena", "ReasonLeft", "(경기장 이탈로 기권)"));
		return;
	}
	if (GetTimeRemaining() <= 0.f)
	{
		const bool bPlayerAhead = GetTeamHealthRatio(PlayerTeam) > GetTeamHealthRatio(OpponentTeam);
		EndMatch(bPlayerAhead, NSLOCTEXT("Arena", "ReasonTimeUp", "(시간 종료, 남은 체력 판정)"));
	}
}


void AMonsterArena::ForfeitMatch()
{
	if (ArenaState == EArenaState::Idle) return;
	EndMatch(false, NSLOCTEXT("Arena", "ReasonForfeit", "(기권)"));
}


// 종료: 보상과 단계 상승(승리 시), 남은 상대 제거, 소환수 자동 교전 끄기, 플레이어 무적·재소환 잠금 해제, 결과 알림
void AMonsterArena::EndMatch(bool bPlayerWon, const FText& Reason)
{
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(CheckTimerHandle);
	ArenaState = EArenaState::Idle;
	NextAllowedStartTime = GetWorld()->GetTimeSeconds() + RematchCooldown;

	const FArenaOpponent* Opponent = GetCurrentOpponent();
	const FText OpponentName = Opponent ? Opponent->OpponentName : FText::GetEmpty();

	for (const TWeakObjectPtr<ABaseMonster>& Weak : OpponentTeam)
	{
		if (ABaseMonster* Monster = Weak.Get())
		{
			Monster->OnMonsterDied.RemoveDynamic(this, &AMonsterArena::HandleCombatantDied);
			if (Monster->IsAlive())
			{
				Monster->Destroy();
			}
		}
	}
	OpponentTeam.Reset();

	for (const TWeakObjectPtr<ABaseMonster>& Weak : PlayerTeam)
	{
		if (ABaseMonster* Summon = Weak.Get())
		{
			Summon->OnMonsterDied.RemoveDynamic(this, &AMonsterArena::HandleCombatantDied);
			// 대전이 끝나면 필드 자동 사냥 설정으로 되돌림
			if (Summon->SummonAIComponent)
			{
				const AActor* SummonOwner = Summon->SummonAIComponent->GetOwnerPlayer();
				const UMonsterCompanionComponent* Companion = SummonOwner ? SummonOwner->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
				if (Companion)
				{
					Companion->ApplyAutoHuntSettings(Summon);
				}
				else
				{
					Summon->SummonAIComponent->bAutoEngage = false;
				}
			}
		}
	}
	PlayerTeam.Reset();

	TArray<FString> RewardParts;
	ARabbit* Rabbit = Challenger.Get();
	if (Rabbit)
	{
		Rabbit->SetCanBeDamaged(true);
		UMonsterCompanionComponent* Companion = Rabbit->FindComponentByClass<UMonsterCompanionComponent>();
		if (Companion)
		{
			Companion->SetSummonLocked(false);
		}

		if (bPlayerWon && Opponent)
		{
			if (UCharacterStatComponent* Stat = Rabbit->FindComponentByClass<UCharacterStatComponent>())
			{
				if (Opponent->RewardEXP > 0)
				{
					Stat->AddExperience(Opponent->RewardEXP);
					RewardParts.Add(FString::Printf(TEXT("경험치 +%d"), Opponent->RewardEXP));
				}
				if (Opponent->RewardMoney > 0)
				{
					Stat->AddMoney(Opponent->RewardMoney);
					RewardParts.Add(FString::Printf(TEXT("돈 +%lld"), Opponent->RewardMoney));
				}
			}
			if (Companion && Opponent->RewardCard && FMath::FRand() < Opponent->RewardCardChance)
			{
				FMonsterCardEntry Card;
				Card.MonsterClass = Opponent->RewardCard;
				Companion->AddCard(Card);

				FString CardName = Opponent->RewardCard->GetName();
				CardName.RemoveFromEnd(TEXT("_C"));
				RewardParts.Add(FString::Printf(TEXT("%s 카드"), *CardName));
			}
		}
	}
	Challenger.Reset();

	FFormatNamedArguments Args;
	Args.Add(TEXT("Opponent"), OpponentName);
	Args.Add(TEXT("Reason"), Reason);
	Args.Add(TEXT("Reward"), FText::FromString(FString::Join(RewardParts, TEXT(", "))));
	UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(bPlayerWon ? WinAnnouncement : LoseAnnouncement, Args), 6.f);

	if (bPlayerWon && Ladder.Num() > 0)
	{
		if (CurrentRank < Ladder.Num() - 1)
		{
			++CurrentRank;
		}
		else
		{
			UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Arena", "LadderComplete", "모든 상대를 이겼습니다! 이제 마지막 상대와 재대전합니다"), 6.f);
		}
	}

	OnMatchEnded.Broadcast(bPlayerWon);
}


// 자리: 지정한 자리가 있으면 돌아가며 쓰고(넘치면 옆으로 조금씩 비켜 섬), 없으면 중심에서 앞(상대)/뒤(나)로 반지름 절반 거리에 가로로 늘어섬
FVector AMonsterArena::GetSpotLocation(bool bOpponentSide, int32 Index, int32 Count) const
{
	const TArray<AActor*>& Spots = bOpponentSide ? OpponentSpots : PlayerSideSpots;
	TArray<const AActor*> ValidSpots;
	for (const AActor* Spot : Spots)
	{
		if (Spot)
		{
			ValidSpots.Add(Spot);
		}
	}

	if (ValidSpots.Num() > 0)
	{
		const FVector Base = ValidSpots[Index % ValidSpots.Num()]->GetActorLocation();
		const int32 Round = Index / ValidSpots.Num();
		return Base + GetActorRightVector() * (120.f * Round);
	}

	const float Side = bOpponentSide ? 1.f : -1.f;
	const float Spread = (Index - (Count - 1) * 0.5f) * 250.f;
	return GetActorLocation() + GetActorForwardVector() * (Side * ArenaRadius * 0.5f) + GetActorRightVector() * Spread;
}


float AMonsterArena::GetTeamHealthRatio(const TArray<TWeakObjectPtr<ABaseMonster>>& Team)
{
	if (Team.Num() == 0) return 0.f;

	float Sum = 0.f;
	for (const TWeakObjectPtr<ABaseMonster>& Weak : Team)
	{
		if (Weak.IsValid() && Weak->IsAlive())
		{
			Sum += Weak->CurrentHP / FMath::Max(Weak->GetMaxHP(), 1.f);
		}
	}
	return Sum / Team.Num();
}


int32 AMonsterArena::CountAlive(const TArray<TWeakObjectPtr<ABaseMonster>>& Team)
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ABaseMonster>& Weak : Team)
	{
		if (Weak.IsValid() && Weak->IsAlive())
		{
			++Count;
		}
	}
	return Count;
}
