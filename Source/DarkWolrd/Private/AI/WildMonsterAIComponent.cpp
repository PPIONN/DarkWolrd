// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/WildMonsterAIComponent.h"
#include "Monster/BaseMonster.h"
#include "Combat/CombatFaction.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/OverlapResult.h"

UWildMonsterAIComponent::UWildMonsterAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 게임 시작: 플레이어 소환수로 스폰된 개체가 아니면 그 자리를 집으로 삼고 AI를 켬
// (소환수는 MonsterCompanionComponent가 스폰을 끝내기 전에 bIsPlayerSummon을 세워두므로 여기서 걸러짐)
void UWildMonsterAIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartWildAI();
	}
}


void UWildMonsterAIComponent::StartWildAI()
{
	const ABaseMonster* SelfMonster = GetOwnerMonster();
	if (!SelfMonster || SelfMonster->bIsPlayerSummon || SelfMonster->Faction == ECombatFaction::Player) return;

	HomeLocation = SelfMonster->GetActorLocation();
	CurrentTarget = nullptr;
	bReturningHome = false;
	bPatrolWalking = false;
	NextPatrolTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(0.5f, PatrolPauseMax);

	StartDecisionLoop();
}


void UWildMonsterAIComponent::StopWildAI()
{
	StopDecisionLoop();
	CurrentTarget = nullptr;
	bReturningHome = false;
}


// 공격받음: 투사체면 쏜 쪽을 공격자로 봄. 귀환 중이 아니고 싸우던 대상이 없으면 공격자를 대상으로 삼음
// (비선공 몬스터도 이렇게 반격을 시작함)
void UWildMonsterAIComponent::NotifyAttacked(AActor* Attacker)
{
	if (!bIsActive || bReturningHome || !IsValid(Attacker)) return;

	if (!Attacker->IsA<APawn>() && Attacker->GetInstigator())
	{
		Attacker = Attacker->GetInstigator();
	}
	if (!CombatFaction::AreHostile(GetOwner(), Attacker) || !CombatFaction::IsAlive(Attacker)) return;

	if (!CombatFaction::IsAlive(CurrentTarget))
	{
		CurrentTarget = Attacker;
	}
}


// 판단: 귀환 중이면 귀환만, 아니면 대상 확인 → (없으면) 감지 → 싸움 / 순찰
void UWildMonsterAIComponent::TickDecision()
{
	ABaseMonster* SelfMonster = GetOwnerMonster();
	AAIController* AICon = GetAIController();
	if (!bIsActive || !SelfMonster || !AICon || !SelfMonster->IsAlive()) return;

	if (bReturningHome)
	{
		UpdateReturnHome(AICon);
		return;
	}

	// 대상이 쓰러졌거나(플레이어 사망 포함) 더 이상 적이 아니면 놓아줌
	if (CurrentTarget && (!CombatFaction::IsAlive(CurrentTarget) || !CombatFaction::AreHostile(SelfMonster, CurrentTarget)))
	{
		CurrentTarget = nullptr;
	}

	const float DistanceFromHome = FVector::Dist2D(SelfMonster->GetActorLocation(), HomeLocation);

	// 집에서 너무 멀리 끌려왔으면 싸움을 포기하고 귀환
	if (DistanceFromHome > LeashRadius)
	{
		BeginReturnHome();
		return;
	}

	if (!CurrentTarget && bAggressive)
	{
		CurrentTarget = FindNearestHostile();
	}

	if (CurrentTarget)
	{
		bPatrolWalking = false;
		EngageTarget(CurrentTarget);
		return;
	}

	// 싸움이 끝났는데 순찰 범위 밖이면 집으로 돌아가서 회복
	if (DistanceFromHome > PatrolRadius * 1.5f)
	{
		BeginReturnHome();
		return;
	}

	UpdatePatrol(AICon);
}


AActor* UWildMonsterAIComponent::FindNearestHostile() const
{
	const AActor* Self = GetOwner();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Self);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Self->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(AggroRadius), QueryParams);

	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!CombatFaction::AreHostile(Self, Candidate) || !CombatFaction::IsAlive(Candidate)) continue;

		const float DistSq = FVector::DistSquared(Self->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Candidate;
		}
	}
	return Nearest;
}


void UWildMonsterAIComponent::BeginReturnHome()
{
	CurrentTarget = nullptr;
	bReturningHome = true;
	bPatrolWalking = false;
	ApplySpeedMultiplier(1.f);
}


// 귀환: 집에 도착하면 귀환 상태를 끝내고 (설정에 따라) HP를 가득 채움. 귀환 중에는 새 적을 찾지 않음
void UWildMonsterAIComponent::UpdateReturnHome(AAIController* AICon)
{
	ABaseMonster* SelfMonster = GetOwnerMonster();
	if (FVector::Dist2D(SelfMonster->GetActorLocation(), HomeLocation) <= 100.f)
	{
		bReturningHome = false;
		AICon->StopMovement();
		if (bHealOnReturn)
		{
			SelfMonster->CurrentHP = SelfMonster->GetMaxHP();
		}
		NextPatrolTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(PatrolPauseMin, PatrolPauseMax);
		return;
	}

	if (AICon->GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		AICon->MoveToLocation(HomeLocation, 50.f);
	}
}


// 순찰: 걷는 중이면 그대로 두고, 도착하면 PatrolPause만큼 쉰 뒤 집 주변의 갈 수 있는 다른 지점을 고름
void UWildMonsterAIComponent::UpdatePatrol(AAIController* AICon)
{
	if (!bPatrol) return;

	ApplySpeedMultiplier(PatrolSpeedMultiplier);

	if (AICon->GetMoveStatus() == EPathFollowingStatus::Moving) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (bPatrolWalking)
	{
		bPatrolWalking = false;
		NextPatrolTime = Now + FMath::FRandRange(PatrolPauseMin, PatrolPauseMax);
		return;
	}
	if (Now < NextPatrolTime) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Destination;
	if (NavSys && NavSys->GetRandomReachablePointInRadius(HomeLocation, PatrolRadius, Destination))
	{
		AICon->MoveToLocation(Destination.Location, 30.f);
		bPatrolWalking = true;
	}
	else
	{
		NextPatrolTime = Now + 1.f;
	}
}
