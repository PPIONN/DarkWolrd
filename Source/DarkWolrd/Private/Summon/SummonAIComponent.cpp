// Fill out your copyright notice in the Description page of Project Settings.

#include "Summon/SummonAIComponent.h"
#include "Monster/BaseMonster.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Combat/AbilityComponent.h"
#include "Character/Rabbit.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

USummonAIComponent::USummonAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void USummonAIComponent::BeginPlay()
{
	Super::BeginPlay();
}


// 액터가 사라질 때 판단 루프와 예약된 타격(HitDelay) 타이머를 전부 정리
void USummonAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}


// 소환 활성화: 판단 루프를 DecisionInterval마다 반복 실행. 스킬은 소환 직후 전부 사용 가능한 상태로 시작
void USummonAIComponent::ActivateSummonAI(AActor* InOwnerPlayer, int32 InFollowSlotIndex)
{
	OwnerPlayer = InOwnerPlayer;
	FollowSlotIndex = InFollowSlotIndex;
	bIsActive = true;
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = false;
	NextActionTime = 0.f;

	if (const ABaseMonster* SelfMonster = Cast<ABaseMonster>(GetOwner()))
	{
		SkillReadyTimes.Init(0.f, SelfMonster->ActiveSkills.Num());
	}

	if (const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		DefaultMaxWalkSpeed = OwnerChar->GetCharacterMovement()->MaxWalkSpeed;
	}
	CurrentSpeedMultiplier = 1.f;
	PlayerStillTime = 0.f;
	bWandering = false;
	bWanderWalking = false;

	GetWorld()->GetTimerManager().SetTimer(DecisionTimerHandle, this, &USummonAIComponent::TickDecision, DecisionInterval, true);
}


// 소환 해제: 모든 우선순위 정보를 지우고 판단 루프를 멈춤
void USummonAIComponent::DeactivateSummonAI()
{
	ApplySpeedMultiplier(1.f);
	bWandering = false;

	bIsActive = false;
	OwnerPlayer = nullptr;
	SelfDefenseTarget = nullptr;
	bSelfDefenseEngaged = false;
	bForceGather = false;

	GetWorld()->GetTimerManager().ClearTimer(DecisionTimerHandle);

	if (AAIController* AICon = GetAIController())
	{
		AICon->StopMovement();
	}
}


// 공격받음 통보: 투사체가 때렸으면 그걸 쏜 쪽을 공격자로 봄. 플레이어 본인이나 다른 소환수는 적으로 취급하지 않음.
// 이미 다른 적과 교전(우선순위 1) 중이면 무시. 아니면 새 공격자를 우선순위 3으로 등록하고,
// 강제 모임 명령은 자기방어를 위해 해제함
void USummonAIComponent::NotifyAttacked(AActor* Attacker)
{
	if (!bIsActive || !IsValid(Attacker)) return;

	if (!Attacker->IsA<APawn>() && Attacker->GetInstigator())
	{
		Attacker = Attacker->GetInstigator();
	}
	if (Attacker == OwnerPlayer) return;
	if (const ABaseMonster* AttackerMonster = Cast<ABaseMonster>(Attacker))
	{
		if (AttackerMonster->bIsPlayerSummon) return;
	}

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


AAIController* USummonAIComponent::GetAIController() const
{
	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		return Cast<AAIController>(OwnerChar->GetController());
	}
	return nullptr;
}


// DecideTarget: 우선순위 규칙대로 지금 싸워야 할 대상을 고름
AActor* USummonAIComponent::DecideTarget()
{
	// 자기방어 대상이 죽거나 무효화되면 등록 정보를 지우고 다음 후보로 넘어감
	if (IsValid(SelfDefenseTarget))
	{
		const ABaseMonster* SelfDefenseMonster = Cast<ABaseMonster>(SelfDefenseTarget);
		if (SelfDefenseMonster && !SelfDefenseMonster->IsAlive())
		{
			SelfDefenseTarget = nullptr;
		}
	}
	else
	{
		SelfDefenseTarget = nullptr;
	}
	if (!SelfDefenseTarget)
	{
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


// TickDecision: DecisionInterval마다 실행. 너무 멀면 순간이동, 대상이 없으면 따라다니거나(플레이어가 가만히 있으면) 배회하고,
// 대상이 있으면 스킬 → 평타 순으로 시도하고 사거리 밖이면 다가감
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
	ApplySpeedMultiplier(1.f);

	// 평타/스킬 동작 중에는 새 행동을 시작하지 않음
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextActionTime) return;

	const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());

	if (TryUseSkill(Target, Distance)) return;

	if (Distance <= AttackRange)
	{
		AICon->StopMovement();
		FaceTarget(Target);

		if (Now - LastAttackTime >= AttackInterval)
		{
			LastAttackTime = Now;
			const ABaseMonster* SelfMonster = Cast<ABaseMonster>(GetOwner());
			StartAttack(Target, 1.f, 0.f,
				SelfMonster ? SelfMonster->BasicAttackMontage : nullptr,
				SelfMonster ? SelfMonster->BasicAttackHitDelay : 0.f);
		}
	}
	else
	{
		AICon->MoveToActor(Target, AttackRange * 0.8f);
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


void USummonAIComponent::ApplySpeedMultiplier(float Multiplier)
{
	if (FMath::IsNearlyEqual(Multiplier, CurrentSpeedMultiplier)) return;

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		CurrentSpeedMultiplier = Multiplier;
		OwnerChar->GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * Multiplier;
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


// 스킬 사용: 배열 앞쪽부터 보고, 쿨타임이 돌아왔고 사거리 안인 첫 스킬을 씀
bool USummonAIComponent::TryUseSkill(AActor* Target, float Distance)
{
	const ABaseMonster* SelfMonster = Cast<ABaseMonster>(GetOwner());
	if (!SelfMonster) return false;

	if (SkillReadyTimes.Num() != SelfMonster->ActiveSkills.Num())
	{
		SkillReadyTimes.Init(0.f, SelfMonster->ActiveSkills.Num());
	}

	const float Now = GetWorld()->GetTimeSeconds();
	for (int32 SkillIndex = 0; SkillIndex < SelfMonster->ActiveSkills.Num(); ++SkillIndex)
	{
		const FSummonActiveSkill& Skill = SelfMonster->ActiveSkills[SkillIndex];
		if (Distance > Skill.Range || Now < SkillReadyTimes[SkillIndex]) continue;

		SkillReadyTimes[SkillIndex] = Now + Skill.Cooldown;

		if (AAIController* AICon = GetAIController())
		{
			AICon->StopMovement();
		}
		FaceTarget(Target);
		StartAttack(Target, Skill.DamageMultiplier, Skill.AreaRadius, Skill.Montage, Skill.HitDelay);
		return true;
	}
	return false;
}


// 공격 시작: 몽타주를 재생하고 동작이 끝날 때까지 다음 행동을 막음. 피해는 HitDelay 뒤 타격 시점에 들어감
void USummonAIComponent::StartAttack(AActor* Target, float DamageMultiplier, float AreaRadius, UAnimMontage* Montage, float HitDelay)
{
	float ActionDuration = HitDelay;
	if (Montage)
	{
		if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
		{
			ActionDuration = FMath::Max(ActionDuration, OwnerChar->PlayAnimMontage(Montage));
		}
	}
	NextActionTime = GetWorld()->GetTimeSeconds() + ActionDuration;

	if (HitDelay <= 0.f)
	{
		ApplyAttackDamage(Target, DamageMultiplier, AreaRadius);
		return;
	}

	// 타격 시점 전에 대상이 사라지거나 소환이 해제되면 피해를 주지 않음
	TWeakObjectPtr<AActor> WeakTarget(Target);
	FTimerHandle HitTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(HitTimerHandle, FTimerDelegate::CreateWeakLambda(this,
		[this, WeakTarget, DamageMultiplier, AreaRadius]()
		{
			if (bIsActive && WeakTarget.IsValid())
			{
				ApplyAttackDamage(WeakTarget.Get(), DamageMultiplier, AreaRadius);
			}
		}), HitDelay, false);
}


// 피해 적용: AreaRadius가 0이면 대상 하나만, 아니면 대상 주변 반경 안의 야생 몬스터 전부
void USummonAIComponent::ApplyAttackDamage(AActor* Target, float DamageMultiplier, float AreaRadius)
{
	const ABaseMonster* SelfMonster = Cast<ABaseMonster>(GetOwner());
	if (!SelfMonster || !IsValid(Target)) return;

	const float Damage = SelfMonster->GetAttackPower() * DamageMultiplier;

	if (AreaRadius <= 0.f)
	{
		DamageEnemy(Cast<ABaseMonster>(Target), Damage);
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	GetWorld()->OverlapMultiByObjectType(Overlaps, Target->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(AreaRadius), QueryParams);

	TSet<ABaseMonster*> AlreadyHit;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ABaseMonster* Enemy = Cast<ABaseMonster>(Overlap.GetActor());
		if (Enemy && !Enemy->bIsPlayerSummon && !AlreadyHit.Contains(Enemy))
		{
			AlreadyHit.Add(Enemy);
			DamageEnemy(Enemy, Damage);
		}
	}
}


// 적 하나에게 피해. Die()가 즉시 Destroy()를 호출하므로, 피해를 주기 전에 필요한 정보를 미리 캡처해둠
void USummonAIComponent::DamageEnemy(ABaseMonster* Enemy, float Damage)
{
	ABaseMonster* SelfMonster = Cast<ABaseMonster>(GetOwner());
	if (!SelfMonster || !IsValid(Enemy) || !Enemy->IsAlive() || Enemy->bIsPlayerSummon) return;

	const float RewardEXP = Enemy->RewardEXP;
	const TSubclassOf<ABaseMonster> SelfClass = SelfMonster->GetClass();

	Enemy->TakeDamage(Damage, FDamageEvent(), GetAIController(), GetOwner());

	if (!Enemy->IsAlive())
	{
		if (IsValid(OwnerPlayer))
		{
			if (UMonsterCompanionComponent* Companion = OwnerPlayer->FindComponentByClass<UMonsterCompanionComponent>())
			{
				Companion->AddSummonEXP(SelfClass, RewardEXP);
			}
		}

		if (SelfDefenseTarget == Enemy)
		{
			SelfDefenseTarget = nullptr;
			bSelfDefenseEngaged = false;
		}
	}
}


void USummonAIComponent::FaceTarget(AActor* Target)
{
	FVector ToTarget = Target->GetActorLocation() - GetOwner()->GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		GetOwner()->SetActorRotation(ToTarget.Rotation());
	}
}
