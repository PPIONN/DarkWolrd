// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/MonsterAIComponent.h"
#include "Monster/BaseMonster.h"
#include "Combat/CombatFaction.h"
#include "Stats/CharacterStatComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UMonsterAIComponent::UMonsterAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 액터가 사라질 때 판단 루프와 예약된 타격(HitDelay) 타이머를 전부 정리
void UMonsterAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}


// 판단 루프 시작: 스킬은 전부 사용 가능한 상태로, 이동속도는 지금 값을 원래 속도로 기억
void UMonsterAIComponent::StartDecisionLoop()
{
	bIsActive = true;
	NextActionTime = 0.f;

	if (const ABaseMonster* SelfMonster = GetOwnerMonster())
	{
		SkillReadyTimes.Init(0.f, SelfMonster->ActiveSkills.Num());
	}

	if (const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		DefaultMaxWalkSpeed = OwnerChar->GetCharacterMovement()->MaxWalkSpeed;
	}
	CurrentSpeedMultiplier = 1.f;

	GetWorld()->GetTimerManager().SetTimer(DecisionTimerHandle, this, &UMonsterAIComponent::TickDecision, DecisionInterval, true);
}


// 판단 루프 정지: 속도를 원래대로 돌리고 이동을 멈춤
void UMonsterAIComponent::StopDecisionLoop()
{
	if (!bIsActive) return;

	ApplySpeedMultiplier(1.f);
	bIsActive = false;

	GetWorld()->GetTimerManager().ClearTimer(DecisionTimerHandle);

	if (AAIController* AICon = GetAIController())
	{
		AICon->StopMovement();
	}
}


AAIController* UMonsterAIComponent::GetAIController() const
{
	if (const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		return Cast<AAIController>(OwnerChar->GetController());
	}
	return nullptr;
}


ABaseMonster* UMonsterAIComponent::GetOwnerMonster() const
{
	return Cast<ABaseMonster>(GetOwner());
}


// 가장자리 거리: 중심 사이 수평 거리에서 두 몸체의 충돌 반지름을 뺀 값 (0 미만이면 0)
float UMonsterAIComponent::GetDistanceToTarget(const AActor* Target) const
{
	if (!Target || !GetOwner()) return TNumericLimits<float>::Max();

	const float CenterDistance = FVector::Dist2D(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	return FMath::Max(0.f, CenterDistance - GetOwner()->GetSimpleCollisionRadius() - Target->GetSimpleCollisionRadius());
}


// 전투: 동작 중이면 대기 → 스킬 → 평타(사거리 안) → 다가가기
void UMonsterAIComponent::EngageTarget(AActor* Target)
{
	AAIController* AICon = GetAIController();
	if (!AICon || !IsValid(Target)) return;

	ApplySpeedMultiplier(1.f);

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextActionTime) return;

	const float Distance = GetDistanceToTarget(Target);

	if (TryUseSkill(Target, Distance)) return;

	if (Distance <= AttackRange)
	{
		AICon->StopMovement();
		FaceTarget(Target);

		if (Now - LastAttackTime >= AttackInterval)
		{
			LastAttackTime = Now;
			const ABaseMonster* SelfMonster = GetOwnerMonster();
			StartAttack(Target, 1.f, AttackRange, 0.f, ESkillAreaCenter::Target, nullptr,
				SelfMonster ? SelfMonster->BasicAttackMontage : nullptr,
				SelfMonster ? SelfMonster->BasicAttackHitDelay : 0.f);
		}
	}
	else
	{
		AICon->MoveToActor(Target, AttackRange * 0.8f);
	}
}


// 스킬 사용: 배열 앞쪽부터 보고, 쿨타임이 돌아왔고 사거리 안인 첫 스킬을 씀
bool UMonsterAIComponent::TryUseSkill(AActor* Target, float Distance)
{
	const ABaseMonster* SelfMonster = GetOwnerMonster();
	if (!SelfMonster) return false;

	// 보스 페이즈 등으로 스킬이 추가되면 기존 스킬의 쿨타임은 그대로 두고 새 스킬만 바로 사용 가능 상태로 늘림
	if (SkillReadyTimes.Num() != SelfMonster->ActiveSkills.Num())
	{
		SkillReadyTimes.SetNumZeroed(SelfMonster->ActiveSkills.Num());
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
		StartAttack(Target, Skill.DamageMultiplier, Skill.Range, Skill.AreaRadius, Skill.AreaCenter, Skill.TelegraphVFX, Skill.Montage, Skill.HitDelay);
		return true;
	}
	return false;
}


// 공격 시작: 몽타주를 재생하고 동작이 끝날 때까지 다음 행동을 막음. 피해는 HitDelay 뒤 타격 시점에 들어감.
// 범위 중심이 "시전 순간 위치"나 "자기 주변"이면 지금 위치를 고정해두고 예고 이펙트를 띄워서, 그 사이 빠져나가면 피할 수 있게 함
void UMonsterAIComponent::StartAttack(AActor* Target, float DamageMultiplier, float AttackReach, float AreaRadius, ESkillAreaCenter AreaCenter,
	UNiagaraSystem* TelegraphVFX, UAnimMontage* Montage, float HitDelay)
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

	const bool bUseFixedCenter = AreaRadius > 0.f && AreaCenter != ESkillAreaCenter::Target;
	const FVector FixedCenter = (AreaCenter == ESkillAreaCenter::Self) ? GetOwner()->GetActorLocation() : Target->GetActorLocation();

	if (TelegraphVFX && AreaRadius > 0.f && HitDelay > 0.f)
	{
		SpawnTelegraph(TelegraphVFX, bUseFixedCenter ? FixedCenter : Target->GetActorLocation(), AreaRadius, HitDelay);
	}

	if (HitDelay <= 0.f)
	{
		ApplyAttackDamage(Target, DamageMultiplier, AttackReach, AreaRadius, bUseFixedCenter, FixedCenter);
		return;
	}

	// 타격 시점 전에 AI가 꺼지면(사망·소환 해제) 피해를 주지 않음.
	// 위치가 고정된 범위 공격은 처음 노린 대상이 사라져도 그 자리에 그대로 떨어짐
	TWeakObjectPtr<AActor> WeakTarget(Target);
	FTimerHandle HitTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(HitTimerHandle, FTimerDelegate::CreateWeakLambda(this,
		[this, WeakTarget, DamageMultiplier, AttackReach, AreaRadius, bUseFixedCenter, FixedCenter]()
		{
			if (bIsActive && (bUseFixedCenter || WeakTarget.IsValid()))
			{
				ApplyAttackDamage(WeakTarget.Get(), DamageMultiplier, AttackReach, AreaRadius, bUseFixedCenter, FixedCenter);
			}
		}), HitDelay, false);
}


// 예고 이펙트: 범위 중심에서 바로 아래 바닥을 찾아 그 위에 띄우고, 타격 순간에 지움
void UMonsterAIComponent::SpawnTelegraph(UNiagaraSystem* TelegraphVFX, const FVector& Center, float AreaRadius, float Duration)
{
	FVector GroundLocation = Center;
	FHitResult GroundHit;
	if (GetWorld()->LineTraceSingleByObjectType(GroundHit, Center, Center - FVector(0.f, 0.f, 1000.f),
		FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		GroundLocation = GroundHit.ImpactPoint + FVector(0.f, 0.f, 2.f);
	}

	UNiagaraComponent* Telegraph = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TelegraphVFX, GroundLocation,
		FRotator::ZeroRotator, FVector(AreaRadius / 100.f), /*bAutoDestroy=*/true);
	if (!Telegraph) return;

	TWeakObjectPtr<UNiagaraComponent> WeakTelegraph(Telegraph);
	FTimerHandle TelegraphTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TelegraphTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakTelegraph]()
		{
			if (WeakTelegraph.IsValid())
			{
				WeakTelegraph->DestroyComponent();
			}
		}), Duration, false);
}


// 피해 적용: AreaRadius가 0이면 대상 하나만, 아니면 범위 중심 반경 안의 적대 진영 전부
void UMonsterAIComponent::ApplyAttackDamage(AActor* Target, float DamageMultiplier, float AttackReach, float AreaRadius, bool bUseFixedCenter, const FVector& FixedCenter)
{
	if (AreaRadius <= 0.f)
	{
		// 공격 동작 사이에 사거리 밖으로 빠졌으면 빗나감
		if (IsValid(Target) && GetDistanceToTarget(Target) <= AttackReach * HitRangeTolerance)
		{
			DamageTarget(Target, DamageMultiplier);
		}
		return;
	}
	if (!bUseFixedCenter && !IsValid(Target)) return;

	const FVector Center = bUseFixedCenter ? FixedCenter : Target->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(AreaRadius), QueryParams);

	TSet<AActor*> AlreadyHit;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (HitActor && !AlreadyHit.Contains(HitActor))
		{
			AlreadyHit.Add(HitActor);
			DamageTarget(HitActor, DamageMultiplier);
		}
	}
}


// 피해 하나: 적대 진영이고 살아있는 대상에게만. Die()가 즉시 Destroy()할 수 있으므로 보상 경험치는 미리 캡처
void UMonsterAIComponent::DamageTarget(AActor* Target, float DamageMultiplier)
{
	ABaseMonster* SelfMonster = GetOwnerMonster();
	if (!SelfMonster || !CombatFaction::IsAlive(Target) || !CombatFaction::AreHostile(SelfMonster, Target)) return;

	float Damage = SelfMonster->GetAttackPower() * DamageMultiplier;
	float RewardEXP = 0.f;

	if (const UCharacterStatComponent* TargetStat = Target->FindComponentByClass<UCharacterStatComponent>())
	{
		// 몬스터대전 상대처럼 플레이어 캐릭터를 무시하는 몬스터는 범위 공격에 휘말린 토끼에게도 피해를 주지 않음
		if (SelfMonster->bIgnoresPlayerCharacter) return;

		// 플레이어 대상: 플레이어 최대 HP와 방어력을 반영하는 몬스터 → 플레이어 공식
		Damage = SelfMonster->CalculatePlayerDamage(TargetStat->GetMaxHP(), TargetStat->GetTotalDefense()) * DamageMultiplier;
	}
	else if (const ABaseMonster* TargetMonster = Cast<ABaseMonster>(Target))
	{
		RewardEXP = TargetMonster->RewardEXP;
	}

	Target->TakeDamage(Damage, FDamageEvent(), GetAIController(), SelfMonster);

	if (!CombatFaction::IsAlive(Target))
	{
		OnTargetKilled(Target, RewardEXP);
	}
}


void UMonsterAIComponent::FaceTarget(AActor* Target)
{
	FVector ToTarget = Target->GetActorLocation() - GetOwner()->GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		GetOwner()->SetActorRotation(ToTarget.Rotation());
	}
}


void UMonsterAIComponent::LockActions(float Duration)
{
	NextActionTime = FMath::Max(NextActionTime, GetWorld()->GetTimeSeconds() + Duration);

	if (AAIController* AICon = GetAIController())
	{
		AICon->StopMovement();
	}
}


void UMonsterAIComponent::ScaleBaseMoveSpeed(float Factor)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || Factor <= 0.f) return;

	if (DefaultMaxWalkSpeed <= 0.f)
	{
		DefaultMaxWalkSpeed = OwnerChar->GetCharacterMovement()->MaxWalkSpeed;
	}
	DefaultMaxWalkSpeed *= Factor;
	OwnerChar->GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * CurrentSpeedMultiplier;
}


void UMonsterAIComponent::ApplySpeedMultiplier(float Multiplier)
{
	if (FMath::IsNearlyEqual(Multiplier, CurrentSpeedMultiplier) || DefaultMaxWalkSpeed <= 0.f) return;

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		CurrentSpeedMultiplier = Multiplier;
		OwnerChar->GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed * Multiplier;
	}
}
