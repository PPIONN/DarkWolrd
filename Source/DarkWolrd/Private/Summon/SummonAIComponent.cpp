// Fill out your copyright notice in the Description page of Project Settings.

#include "Summon/SummonAIComponent.h"
#include "AI/WildMonsterAIComponent.h"
#include "Monster/BaseMonster.h"
#include "Combat/CombatFaction.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Combat/AbilityComponent.h"
#include "Character/Rabbit.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"

USummonAIComponent::USummonAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 소환 활성화: 야생 AI가 돌고 있었다면 끄고, 우선순위 정보를 비운 채 판단 루프를 시작
void USummonAIComponent::ActivateSummonAI(AActor* InOwnerPlayer, int32 InFollowSlotIndex)
{
	if (ABaseMonster* SelfMonster = GetOwnerMonster())
	{
		if (SelfMonster->WildAIComponent)
		{
			SelfMonster->WildAIComponent->StopWildAI();
		}
	}

	OwnerPlayer = InOwnerPlayer;
	FollowSlotIndex = InFollowSlotIndex;
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = false;
	PlayerStillTime = 0.f;
	bWandering = false;
	bWanderWalking = false;

	StartDecisionLoop();
}


// 소환 해제: 모든 우선순위 정보를 지우고 판단 루프를 멈춤
void USummonAIComponent::DeactivateSummonAI()
{
	StopDecisionLoop();

	OwnerPlayer = nullptr;
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = false;
	bWandering = false;
}


// 공격받음 통보: 투사체가 때렸으면 그걸 쏜 쪽을 공격자로 봄. 적대 진영이 아니면(플레이어, 다른 소환수) 무시.
// 이미 다른 적과 교전(우선순위 1) 중이면 무시. 아니면 새 공격자를 우선순위 3으로 등록하고,
// 강제 모임 명령은 자기방어를 위해 해제함
void USummonAIComponent::NotifyAttacked(AActor* Attacker)
{
	if (!bIsActive || !IsValid(Attacker)) return;

	if (!Attacker->IsA<APawn>() && Attacker->GetInstigator())
	{
		Attacker = Attacker->GetInstigator();
	}
	if (!CombatFaction::AreHostile(GetOwner(), Attacker)) return;

	if (bSelfDefenseEngaged && IsValid(SelfDefenseTarget)) return;

	SelfDefenseTarget = Attacker;
	bSelfDefenseEngaged = false;
	bForceGather = false;
}


// "4+1": 모든 우선순위 초기화, 플레이어 곁으로 강제 집결
void USummonAIComponent::CommandGather()
{
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = true;
}


// "4+2": 플레이어가 공격중인 대상만 남기고 나머지 우선순위(자기방어 등) 초기화
void USummonAIComponent::CommandAttackPlayerTarget()
{
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = false;
}


// DecideTarget: 우선순위 규칙대로 지금 싸워야 할 대상을 고름
AActor* USummonAIComponent::DecideTarget()
{
	// 자기방어 대상이 쓰러지거나 사라지면 등록 정보를 지우고 다음 후보로 넘어감
	if (!CombatFaction::IsAlive(SelfDefenseTarget))
	{
		SelfDefenseTarget = nullptr;
		bSelfDefenseEngaged = false;
	}

	// 우선순위 1: 이미 교전을 시작한 자기방어 대상
	if (SelfDefenseTarget && bSelfDefenseEngaged)
	{
		return SelfDefenseTarget;
	}

	// 우선순위 2: 플레이어가 락온 중인 대상 (강제 모임 명령 중에는 무시).
	// 막 공격해온 적(우선순위 3)이 있어도 락온 대상과의 싸움을 계속하고, 그 적은 대기시켜 둠
	if (!bForceGather && IsValid(OwnerPlayer))
	{
		if (UAbilityComponent* PlayerAbility = OwnerPlayer->FindComponentByClass<UAbilityComponent>())
		{
			if (PlayerAbility->IsLockedOn() && IsValid(PlayerAbility->LockedTarget))
			{
				return PlayerAbility->LockedTarget;
			}
		}
	}

	// 우선순위 3: 나를 공격한 적. 대응을 시작하는 순간 우선순위 1로 승격
	if (SelfDefenseTarget)
	{
		bSelfDefenseEngaged = true;
		return SelfDefenseTarget;
	}

	// 후보 없음: 따라다니기/배회 상태
	return nullptr;
}


// TickDecision: 너무 멀면 순간이동, 대상이 없으면 따라다니거나(플레이어가 가만히 있으면) 배회, 대상이 있으면 싸움
void USummonAIComponent::TickDecision()
{
	if (!bIsActive || !IsValid(GetOwner())) return;

	AAIController* AICon = GetAIController();
	if (!AICon) return;

	if (IsValid(OwnerPlayer))
	{
		PlayerStillTime = OwnerPlayer->GetVelocity().Size2D() < 10.f ? PlayerStillTime + DecisionInterval : 0.f;

		if (FVector::Dist(GetOwner()->GetActorLocation(), OwnerPlayer->GetActorLocation()) > TeleportDistance)
		{
			TeleportToPlayer();
			return;
		}
	}

	AActor* Target = DecideTarget();

	if (!Target)
	{
		if (IsPlayerIdle())
		{
			UpdateWander(AICon);
		}
		else
		{
			bWandering = false;
			UpdateFollow(AICon);
		}
		return;
	}

	bWandering = false;
	EngageTarget(Target);
}


// 처치 보상: 몬스터를 쓰러뜨렸으면 소환주에게 이 종족의 경험치 지급을 요청하고, 자기방어 대상이었다면 정리
void USummonAIComponent::OnTargetKilled(AActor* Victim, float VictimRewardEXP)
{
	if (VictimRewardEXP > 0.f && IsValid(OwnerPlayer))
	{
		if (UMonsterCompanionComponent* Companion = OwnerPlayer->FindComponentByClass<UMonsterCompanionComponent>())
		{
			Companion->AddSummonEXP(GetOwner()->GetClass(), VictimRewardEXP);
		}
	}

	if (SelfDefenseTarget == Victim)
	{
		SelfDefenseTarget = nullptr;
		bSelfDefenseEngaged = false;
	}
}


// 따라다니기: 플레이어 뒤 자기 자리로 이동. 플레이어와 많이 벌어졌으면 이동속도를 올려서 따라잡음
void USummonAIComponent::UpdateFollow(AAIController* AICon)
{
	if (!IsValid(OwnerPlayer)) return;

	const FVector MyLocation = GetOwner()->GetActorLocation();
	const bool bFarBehind = FVector::Dist2D(MyLocation, OwnerPlayer->GetActorLocation()) > CatchUpDistance;
	ApplySpeedMultiplier(bFarBehind ? CatchUpSpeedMultiplier : 1.f);

	const FVector SlotLocation = GetFollowSlotLocation();
	if (FVector::Dist2D(MyLocation, SlotLocation) > FollowAcceptanceRadius)
	{
		AICon->MoveToLocation(SlotLocation, FollowAcceptanceRadius * 0.5f);
	}
}


// 자리 배치: 플레이어가 바라보는 방향 기준으로 0 = 바로 뒤, 1/2 = 뒤쪽 양옆 대각선
FVector USummonAIComponent::GetFollowSlotLocation() const
{
	static const float SlotYawOffsets[] = { 180.f, 135.f, 225.f };
	const float YawOffset = SlotYawOffsets[FMath::Abs(FollowSlotIndex) % UE_ARRAY_COUNT(SlotYawOffsets)];

	const FRotator SlotDirection(0.f, OwnerPlayer->GetActorRotation().Yaw + YawOffset, 0.f);
	return OwnerPlayer->GetActorLocation() + SlotDirection.Vector() * FollowDistance;
}


// 순간이동: 자기 자리로 먼저 시도하고, 막혀 있으면 플레이어 위치 근처로 (TeleportTo가 겹치지 않는 자리를 찾아줌)
void USummonAIComponent::TeleportToPlayer()
{
	if (!IsValid(OwnerPlayer)) return;

	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bWandering = false;
	ApplySpeedMultiplier(1.f);

	if (AAIController* AICon = GetAIController())
	{
		AICon->StopMovement();
	}

	const FRotator FacingRotation(0.f, OwnerPlayer->GetActorRotation().Yaw, 0.f);
	if (!GetOwner()->TeleportTo(GetFollowSlotLocation(), FacingRotation))
	{
		GetOwner()->TeleportTo(OwnerPlayer->GetActorLocation(), FacingRotation);
	}
}


// 플레이어가 가만히 있는지: 실제로 멈춰 있고(속도 기준), 토끼라면 입력도 IdleWanderDelay 동안 없어야 함
// (시점 회전·인벤토리는 입력으로 치지 않으므로 카메라만 돌려도 배회는 계속됨)
bool USummonAIComponent::IsPlayerIdle() const
{
	if (PlayerStillTime < IdleWanderDelay) return false;

	if (const ARabbit* Rabbit = Cast<ARabbit>(OwnerPlayer))
	{
		return Rabbit->GetTimeSinceLastInput() >= IdleWanderDelay;
	}
	return true;
}


// 배회: 걷는 중이면 그대로 두고, 도착하면 WanderPause만큼 쉰 뒤 플레이어 주변의 갈 수 있는 다른 지점을 고름.
// 처음 배회를 시작할 때는 세 마리가 동시에 움직이지 않도록 출발 시각을 조금씩 어긋나게 함
void USummonAIComponent::UpdateWander(AAIController* AICon)
{
	if (!IsValid(OwnerPlayer)) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (!bWandering)
	{
		bWandering = true;
		bWanderWalking = false;
		NextWanderTime = Now + FMath::FRandRange(0.5f, 2.f);
		ApplySpeedMultiplier(WanderSpeedMultiplier);
	}

	if (AICon->GetMoveStatus() == EPathFollowingStatus::Moving) return;

	if (bWanderWalking)
	{
		bWanderWalking = false;
		NextWanderTime = Now + FMath::FRandRange(WanderPauseMin, WanderPauseMax);
		return;
	}
	if (Now < NextWanderTime) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	const FVector PlayerLocation = OwnerPlayer->GetActorLocation();
	for (int32 Attempt = 0; Attempt < 5; ++Attempt)
	{
		FNavLocation Candidate;
		if (NavSys->GetRandomReachablePointInRadius(PlayerLocation, WanderRadius, Candidate) &&
			FVector::Dist2D(Candidate.Location, PlayerLocation) >= WanderMinDistanceFromPlayer)
		{
			AICon->MoveToLocation(Candidate.Location, 30.f);
			bWanderWalking = true;
			return;
		}
	}

	// 적당한 지점을 못 찾았으면 잠깐 뒤에 다시 시도
	NextWanderTime = Now + 1.f;
}
