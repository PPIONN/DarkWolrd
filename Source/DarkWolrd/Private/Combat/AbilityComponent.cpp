// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/AbilityComponent.h"
#include "Combat/MagicOrbProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Stats/CharacterStatComponent.h"
#include "Monster/BaseMonster.h"
#include "TimerManager.h"
#include "EngineUtils.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 게임 시작 시: 같은 액터에 붙은 CharacterStatComponent의 레벨업 이벤트를 구독해서 스킬 포인트를 지급받음.
// 서로의 존재를 몰라도 되게(느슨한 결합) 델리게이트로 연결
void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* OwnerActor = GetOwner())
	{
		CachedStatComponent = OwnerActor->FindComponentByClass<UCharacterStatComponent>();
		if (CachedStatComponent)
		{
			CachedStatComponent->OnLevelUp.AddDynamic(this, &UAbilityComponent::HandleCharacterLevelUp);
		}
	}
}


// 공격속도 배율 조회: 스탯 컴포넌트를 못 찾으면 1(정상 속도)로 취급
float UAbilityComponent::GetAttackSpeedMultiplier() const
{
	return CachedStatComponent ? CachedStatComponent->GetAttackSpeedMultiplier() : 1.f;
}


// 캐릭터 레벨업 1회당 스킬 포인트 1개 지급
void UAbilityComponent::HandleCharacterLevelUp()
{
	AddSkillPoints(1);
}


void UAbilityComponent::AddSkillPoints(int32 Amount)
{
	if (Amount > 0)
	{
		AvailableSkillPoints += Amount;
	}
}


// 스킬 레벨 투자: 포인트가 있고 최대 레벨 미달이면 1 올리고 포인트 1 소비.
// 아직 배우지 않은 스킬(레벨 0)도 이 함수로 처음 배우게 됨(0 -> 1)
bool UAbilityComponent::TryLevelUpSkill(FName AbilityID)
{
	if (AvailableSkillPoints <= 0) return false;

	const int32 CurrentLevel = SkillLevels.FindRef(AbilityID);
	if (CurrentLevel >= MaxSkillLevel) return false;

	SkillLevels.Add(AbilityID, CurrentLevel + 1);
	--AvailableSkillPoints;
	return true;
}


// IsOnCooldown: 쿨다운 종료 시각이 아직 안 지났으면 true
bool UAbilityComponent::IsOnCooldown(FName AbilityID) const
{
	const float* EndTime = CooldownEndTimes.Find(AbilityID);
	return EndTime && GetWorld()->GetTimeSeconds() < *EndTime;
}


// GetRemainingCooldown: 남은 쿨다운 시간(초). 쿨다운 중이 아니면 0
float UAbilityComponent::GetRemainingCooldown(FName AbilityID) const
{
	const float* EndTime = CooldownEndTimes.Find(AbilityID);
	if (!EndTime) return 0.f;

	return FMath::Max(*EndTime - GetWorld()->GetTimeSeconds(), 0.f);
}


// 어빌리티 입력 처리: 슬롯 상태에 따라 캐스팅 시작 또는 발사를 수행. 그 외 조합은 전부 무시
bool UAbilityComponent::TryActivateAbility(FName AbilityID)
{
	switch (CastState)
	{
	case EAbilityCastState::Idle:
	{
		const FAbilityData* Data = Abilities.Find(AbilityID);
		if (!Data || !Data->ProjectileClass) return false;
		if (IsOnCooldown(AbilityID)) return false;

		if (Data->bInstantCast)
		{
			CastInstantly(AbilityID, *Data);
		}
		else
		{
			StartCasting(AbilityID, *Data);
		}

		if (Data->Cooldown > 0.f)
		{
			// 공격속도가 빠를수록 쿨다운도 그만큼 짧아짐
			CooldownEndTimes.Add(AbilityID, GetWorld()->GetTimeSeconds() + Data->Cooldown / GetAttackSpeedMultiplier());
		}
		return true;
	}

	case EAbilityCastState::Casting:
		// 캐스팅 슬롯이 이미 점유된 상태 - 같은 키든 다른 키든 무시
		return false;

	case EAbilityCastState::Ready:
		if (AbilityID == ActiveAbilityID)
		{
			FireReadyAbility();
			return true;
		}
		// 다른 어빌리티가 준비된 상태에서는 이 입력을 무시 (캐스팅 슬롯은 하나뿐)
		return false;
	}

	return false;
}


// 즉시 시전: 준비 슬롯을 점유하지 않고(캐스팅 슬롯은 Idle을 유지), 오브를 전부 즉시 생성해 곧바로 발사한다.
// 기본공격처럼 "누르면 그대로 나가는" 어빌리티용 — bInstantCast=true인 경우에만 호출됨
void UAbilityComponent::CastInstantly(FName AbilityID, const FAbilityData& Data)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	if (Data.CastMontage)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
			{
				// 공격속도가 빠를수록 몽타주 재생속도도 그만큼 빨라짐
				AnimInstance->Montage_Play(Data.CastMontage, GetAttackSpeedMultiplier());
			}
		}
	}

	// 발사 순간을 공격 동작에 맞춤: LaunchDelay가 있으면 그만큼(공격속도가 빠르면 더 짧게) 기다렸다가 생성·발사
	const float Delay = Data.LaunchDelay / GetAttackSpeedMultiplier();
	if (Delay > KINDA_SMALL_NUMBER)
	{
		FTimerHandle LaunchHandle;
		GetWorld()->GetTimerManager().SetTimer(LaunchHandle,
			FTimerDelegate::CreateUObject(this, &UAbilityComponent::LaunchInstantOrbs, AbilityID), Delay, false);
	}
	else
	{
		LaunchInstantOrbs(AbilityID);
	}
}


// 즉시 시전 오브를 생성해서 조준 지점으로 발사. LaunchSocket이 메시에 있으면 그 위치(예: 오른손)에서, 없으면 캐릭터 중심에서 생김
void UAbilityComponent::LaunchInstantOrbs(FName AbilityID)
{
	AActor* OwnerActor = GetOwner();
	const FAbilityData* Data = Abilities.Find(AbilityID);
	if (!OwnerActor || !Data || !Data->ProjectileClass) return;

	FVector SpawnLocation = OwnerActor->GetActorLocation();
	if (!Data->LaunchSocket.IsNone())
	{
		const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
		const USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
		if (Mesh && Mesh->DoesSocketExist(Data->LaunchSocket))
		{
			SpawnLocation = Mesh->GetSocketLocation(Data->LaunchSocket);
		}
	}

	const int32 OrbCount = FMath::Max(Data->BaseOrbCount + BonusOrbCountFromLevel, 1);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AMagicOrbProjectile*> NewOrbs;
	for (int32 i = 0; i < OrbCount; ++i)
	{
		if (AMagicOrbProjectile* NewOrb = GetWorld()->SpawnActor<AMagicOrbProjectile>(
			Data->ProjectileClass, SpawnLocation, OwnerActor->GetActorRotation(), SpawnParams))
		{
			NewOrb->SourceAbilityID = AbilityID;
			// 마력·지능(장비 포함)에 따른 피해 배율. 기본 능력치면 1배
			NewOrb->Damage *= CachedStatComponent ? CachedStatComponent->GetOutgoingDamageMultiplier() : 1.f;
			NewOrbs.Add(NewOrb);
		}
	}

	const FVector AimPoint = GetAimPoint();
	AActor* HomingTarget = IsLockedOn() ? LockedTarget : nullptr;
	for (AMagicOrbProjectile* Orb : NewOrbs)
	{
		const FVector Direction = (AimPoint - Orb->GetActorLocation()).GetSafeNormal();
		// 락온 중이면 유도탄이 되어 사실상 확정으로 맞으므로 피해를 LockOnDamageMultiplier(기본 70%)로 줄임
		if (HomingTarget)
		{
			Orb->Damage *= LockOnDamageMultiplier;
		}
		Orb->Launch(Direction, HomingTarget);
	}
}


// 캐스팅 시작: 슬롯을 점유하고, 오브를 하나씩 순차 스폰하는 타이머를 건다
void UAbilityComponent::StartCasting(FName AbilityID, const FAbilityData& Data)
{
	CastState = EAbilityCastState::Casting;
	ActiveAbilityID = AbilityID;
	CurrentAbilityData = Data;
	OrbsSpawnedSoFar = 0;
	TotalOrbsToSpawn = FMath::Max(Data.BaseOrbCount + BonusOrbCountFromLevel, 1);
	PendingOrbs.Reset();

	if (Data.CastMontage)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Play(Data.CastMontage, GetAttackSpeedMultiplier());
			}
		}
	}

	const float SpawnInterval = FMath::Max(Data.CastTime / TotalOrbsToSpawn, 0.01f);
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &UAbilityComponent::SpawnNextOrbTick, SpawnInterval, true);
}


// 오브 순차 스폰: 캐릭터 주변 원형 배치 위치에 하나씩 스폰. 목표 개수에 도달하면 Ready로 전환
void UAbilityComponent::SpawnNextOrbTick()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !CurrentAbilityData.ProjectileClass)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
		CastState = EAbilityCastState::Idle;
		ActiveAbilityID = NAME_None;
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AMagicOrbProjectile* NewOrb = GetWorld()->SpawnActor<AMagicOrbProjectile>(
		CurrentAbilityData.ProjectileClass, OwnerActor->GetActorLocation(), OwnerActor->GetActorRotation(), SpawnParams);

	if (NewOrb)
	{
		NewOrb->SourceAbilityID = ActiveAbilityID;
		NewOrb->Damage *= CachedStatComponent ? CachedStatComponent->GetOutgoingDamageMultiplier() : 1.f;

		const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
		NewOrb->AttachToComponent(OwnerActor->GetRootComponent(), AttachRules);
		NewOrb->SetActorRelativeLocation(GetOrbRingOffset(OrbsSpawnedSoFar, TotalOrbsToSpawn, CurrentAbilityData));
		PendingOrbs.Add(NewOrb);
	}

	++OrbsSpawnedSoFar;

	if (OrbsSpawnedSoFar >= TotalOrbsToSpawn)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
		CastState = EAbilityCastState::Ready;
	}
}


// 발사: 준비된 오브 전부를 조준 지점을 향해 동시에 발사하고 슬롯을 비움
void UAbilityComponent::FireReadyAbility()
{
	const FVector AimPoint = GetAimPoint();
	AActor* HomingTarget = IsLockedOn() ? LockedTarget : nullptr;

	for (AMagicOrbProjectile* Orb : PendingOrbs)
	{
		if (!Orb) continue;

		const FVector Direction = (AimPoint - Orb->GetActorLocation()).GetSafeNormal();
		// 차지 스킬도 평타와 같은 규칙: 락온 중에 쏘면 피해 LockOnDamageMultiplier배
		if (HomingTarget)
		{
			Orb->Damage *= LockOnDamageMultiplier;
		}
		Orb->Launch(Direction, HomingTarget);
	}

	PendingOrbs.Reset();
	CastState = EAbilityCastState::Idle;
	ActiveAbilityID = NAME_None;
}


// 오브 배치 위치 계산: 캐릭터 주변 원 위에 오브 개수만큼 균등 분배한 상대 위치를 반환
FVector UAbilityComponent::GetOrbRingOffset(int32 Index, int32 Total, const FAbilityData& Data) const
{
	const float AngleStep = 360.f / FMath::Max(Total, 1);
	const float AngleDegrees = AngleStep * Index;
	const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

	return FVector(
		Data.OrbRingRadius * FMath::Cos(AngleRadians),
		Data.OrbRingRadius * FMath::Sin(AngleRadians),
		Data.OrbRingHeight);
}


// 조준 지점 계산: 락온 대상이 있으면 그 위치(+높이 보정)를, 없으면 커서 조준(bAimAtCursor) 또는 화면 중앙 트레이스 결과를 반환
FVector UAbilityComponent::GetAimPoint() const
{
	if (IsValidLockOnTarget(LockedTarget))
	{
		return LockedTarget->GetActorLocation() + FVector(0.f, 0.f, LockOnHeightOffset);
	}

	if (bAimAtCursor)
	{
		// 커서 조준: 커서가 적 위(또는 CursorSnapRadius 안)면 그 적을, 아니면 커서 쪽으로 수평하게 멀리.
		// 커서가 가리키는 바닥 점을 그대로 겨누면 오브가 바닥에 꽂히므로 캐릭터 높이로 수평하게 날림
		const AActor* OwnerActor = GetOwner();
		if (!OwnerActor) return FVector::ZeroVector;
		const FVector Origin = OwnerActor->GetActorLocation();

		FVector Direction = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
		FVector PlanePoint;
		AActor* Hovered = nullptr;
		if (GetCursorAim(PlanePoint, Hovered))
		{
			AActor* AimTarget = IsValidLockOnTarget(Hovered) ? Hovered : FindTargetNear(PlanePoint, CursorSnapRadius);
			if (AimTarget)
			{
				return AimTarget->GetActorLocation() + FVector(0.f, 0.f, LockOnHeightOffset);
			}
			const FVector ToCursor = (PlanePoint - Origin).GetSafeNormal2D();
			if (!ToCursor.IsNearlyZero())
			{
				Direction = ToCursor;
			}
		}
		return Origin + Direction * AimTraceDistance;
	}

	FVector TraceEnd;
	const FHitResult Hit = PerformAimTrace(TraceEnd);
	return Hit.bBlockingHit ? FVector(Hit.ImpactPoint) : TraceEnd;
}


// 커서 조준점: 카메라에서 커서를 지나는 광선이 캐릭터 중심 높이의 수평면과 만나는 점. 커서 아래 액터도 함께 찾음
bool UAbilityComponent::GetCursorAim(FVector& OutPlanePoint, AActor*& OutHoveredActor) const
{
	OutHoveredActor = nullptr;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC || !PC->ShouldShowMouseCursor()) return false;

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) || RayDirection.Z > -KINDA_SMALL_NUMBER) return false;

	const float PlaneZ = OwnerPawn->GetActorLocation().Z;
	OutPlanePoint = RayOrigin + RayDirection * ((PlaneZ - RayOrigin.Z) / RayDirection.Z);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerPawn);
	for (AMagicOrbProjectile* Orb : PendingOrbs)
	{
		if (Orb)
		{
			QueryParams.AddIgnoredActor(Orb);
		}
	}
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, RayOrigin, RayOrigin + RayDirection * (AimTraceDistance * 2.f), ECC_Visibility, QueryParams))
	{
		OutHoveredActor = Hit.GetActor();
	}
	return true;
}


AActor* UAbilityComponent::FindTargetNear(const FVector& Point, float Radius) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	AActor* BestTarget = nullptr;
	float BestDistSq = FMath::Square(Radius);
	for (TActorIterator<ABaseMonster> It(GetWorld()); It; ++It)
	{
		ABaseMonster* Monster = *It;
		if (!IsValidLockOnTarget(Monster)) continue;
		if (FVector::Dist(OwnerActor->GetActorLocation(), Monster->GetActorLocation()) > LockOnRange) continue;

		const float DistSq = FVector::DistSquared2D(Point, Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}
	return BestTarget;
}


AActor* UAbilityComponent::FindTargetInDirection(const FVector& Forward, float MaxAngle) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	const FVector Origin = OwnerActor->GetActorLocation();
	const float MinDot = MaxAngle >= 180.f ? -2.f : FMath::Cos(FMath::DegreesToRadians(MaxAngle));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	AActor* BestTarget = nullptr;
	float BestDistSq = FMath::Square(LockOnRange);
	for (TActorIterator<ABaseMonster> It(GetWorld()); It; ++It)
	{
		ABaseMonster* Monster = *It;
		if (!IsValidLockOnTarget(Monster)) continue;

		const FVector ToMonster = Monster->GetActorLocation() - Origin;
		const float DistSq = ToMonster.SizeSquared();
		if (DistSq >= BestDistSq || FVector::DotProduct(Forward, ToMonster.GetSafeNormal2D()) < MinDot) continue;

		FCollisionQueryParams SightParams = QueryParams;
		SightParams.AddIgnoredActor(Monster);
		if (GetWorld()->LineTraceTestByChannel(Origin, Monster->GetActorLocation(), ECC_Visibility, SightParams)) continue;

		BestDistSq = DistSq;
		BestTarget = Monster;
	}
	return BestTarget;
}


// 화면 중앙 조준 트레이스 실행: 소유 액터의 카메라 위치에서 카메라 정면(피치 제한 적용)으로 트레이스.
// OutTraceEnd에는 트레이스 목표 지점(아무것도 안 맞았을 때 쓸 값)을 채워줌
FHitResult UAbilityComponent::PerformAimTrace(FVector& OutTraceEnd) const
{
	FHitResult Hit;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		OutTraceEnd = FVector::ZeroVector;
		return Hit;
	}

	UCameraComponent* OwnerCamera = OwnerActor->FindComponentByClass<UCameraComponent>();
	const FVector TraceStart = OwnerCamera ? OwnerCamera->GetComponentLocation() : OwnerActor->GetActorLocation();

	// 카메라 회전 그대로 쓰면 내려다볼 때 너무 가파르게(직각으로) 아래를 향하므로, 아래쪽 피치를 제한해서 더 수평에 가깝게 만듦
	FRotator TraceRotation = OwnerCamera ? OwnerCamera->GetComponentRotation() : OwnerActor->GetActorRotation();
	TraceRotation.Pitch = FMath::Max(TraceRotation.Pitch, -MaxAimDownPitch);
	const FVector TraceForward = TraceRotation.Vector();

	OutTraceEnd = TraceStart + TraceForward * AimTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	for (AMagicOrbProjectile* Orb : PendingOrbs)
	{
		if (Orb)
		{
			QueryParams.AddIgnoredActor(Orb);
		}
	}

	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, OutTraceEnd, ECC_Visibility, QueryParams);
	return Hit;
}


// 락온 대상 유효성 확인: 살아있는 몬스터이고, 내 소환수가 아닌 적대 진영인지
bool UAbilityComponent::IsValidLockOnTarget(AActor* Target) const
{
	if (!IsValid(Target)) return false;

	const ABaseMonster* Monster = Cast<ABaseMonster>(Target);
	return Monster && Monster->IsAlive() && CombatFaction::AreHostile(GetOwner(), Monster);
}


// 락온 토글: 락온 중이면 해제, 아니면 화면 중앙에 가장 가까운 적대 몬스터에게 락온.
// (대상이 이미 죽었거나 멀어진 상태면 "락온 중"이 아니므로 해제 대신 새 대상을 찾음)
void UAbilityComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		LockedTarget = nullptr;
		return;
	}

	LockedTarget = FindLockOnTarget();
}


// 락온 대상 찾기: 화면 중앙에 정확히 맞은 몬스터가 우선, 없으면 카메라 정면 원뿔 안에서 화면 중앙에 가장 가까운 몬스터
AActor* UAbilityComponent::FindLockOnTarget() const
{
	// 커서 조준: 커서 아래 적 → 커서 근처(CursorLockOnRadius) 적 → 커서 방향 60° 안 가장 가까운 적 → 범위 안 가장 가까운 적.
	// 처음에는 마지막을 '몸 앞 60°'로 했는데, 공격하면 몸이 커서 쪽으로 돌아서 커서가 적과 동떨어져 있으면 락온이 계속 실패했음
	if (bAimAtCursor && GetOwner())
	{
		FVector PlanePoint;
		AActor* Hovered = nullptr;
		if (GetCursorAim(PlanePoint, Hovered))
		{
			if (IsValidLockOnTarget(Hovered)
				&& FVector::Dist(GetOwner()->GetActorLocation(), Hovered->GetActorLocation()) <= LockOnRange)
			{
				return Hovered;
			}
			if (AActor* NearCursor = FindTargetNear(PlanePoint, CursorLockOnRadius))
			{
				return NearCursor;
			}
			const FVector ToCursor = (PlanePoint - GetOwner()->GetActorLocation()).GetSafeNormal2D();
			if (AActor* InCursorDirection = ToCursor.IsNearlyZero() ? nullptr : FindTargetInDirection(ToCursor, 60.f))
			{
				return InCursorDirection;
			}
		}
		return FindTargetInDirection(GetOwner()->GetActorForwardVector().GetSafeNormal2D(), 180.f);
	}

	FVector TraceEnd;
	const FHitResult Hit = PerformAimTrace(TraceEnd);
	if (Hit.bBlockingHit && IsValidLockOnTarget(Hit.GetActor()))
	{
		return Hit.GetActor();
	}

	AActor* OwnerActor = GetOwner();
	const UCameraComponent* OwnerCamera = OwnerActor ? OwnerActor->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!OwnerCamera) return nullptr;

	const FVector ViewLocation = OwnerCamera->GetComponentLocation();
	const FVector ViewForward = OwnerCamera->GetForwardVector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(LockOnMaxAngle));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	AActor* BestTarget = nullptr;
	float BestDot = MinDot;

	for (TActorIterator<ABaseMonster> It(GetWorld()); It; ++It)
	{
		ABaseMonster* Monster = *It;
		if (!IsValidLockOnTarget(Monster)) continue;
		if (FVector::Dist(OwnerActor->GetActorLocation(), Monster->GetActorLocation()) > LockOnRange) continue;

		const FVector ToMonster = (Monster->GetActorLocation() - ViewLocation).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewForward, ToMonster);
		if (Dot <= BestDot) continue;

		// 벽 등에 가려진 몬스터는 제외
		FCollisionQueryParams SightParams = QueryParams;
		SightParams.AddIgnoredActor(Monster);
		if (GetWorld()->LineTraceTestByChannel(ViewLocation, Monster->GetActorLocation(), ECC_Visibility, SightParams)) continue;

		BestDot = Dot;
		BestTarget = Monster;
	}

	return BestTarget;
}


// 현재 유효한 락온 대상이 있는지 (살아있는 적대 몬스터이고, 너무 멀어지지 않았음)
bool UAbilityComponent::IsLockedOn() const
{
	if (!IsValidLockOnTarget(LockedTarget) || !GetOwner()) return false;

	return FVector::Dist(GetOwner()->GetActorLocation(), LockedTarget->GetActorLocation()) <= LockOnRange * 1.5f;
}
