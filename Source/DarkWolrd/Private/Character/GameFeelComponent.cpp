// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/GameFeelComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UGameFeelComponent::UGameFeelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 이동이 끝난 뒤의 속도로 기울기·착지를 판단
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}


// 원래 메시 크기·회전을 기억해 두고, 이후로는 그 위에 연출을 곱해서 적용
void UGameFeelComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Mesh = Character->GetMesh();
	}
	if (Mesh)
	{
		BaseScale = Mesh->GetRelativeScale3D();
		BaseRotation = Mesh->GetRelativeRotation().Quaternion();
	}
}


void UGameFeelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Mesh) return;

	const float StepTime = FMath::Min(DeltaTime, 0.1f);
	UpdateAirState();
	if (bEnableLean)
	{
		UpdateLean(StepTime);
	}
	StepSpring(Squash, SquashVelocity, 0.f, SquashFrequency, SquashDampingRatio, StepTime);

	// 모든 연출이 가라앉았으면 원래 상태로 한 번 맞춘 뒤 매 프레임 메시를 건드리지 않음
	const bool bSettled = FMath::Abs(Squash) < 0.001f && FMath::Abs(SquashVelocity) < 0.01f
		&& FMath::Abs(LeanRight) < 0.02f && FMath::Abs(LeanRightVelocity) < 0.05f
		&& FMath::Abs(LeanForward) < 0.02f && FMath::Abs(LeanForwardVelocity) < 0.05f
		&& AppearScale == 1.f;
	if (bSettled)
	{
		if (!bAtRest)
		{
			Squash = SquashVelocity = LeanRight = LeanRightVelocity = LeanForward = LeanForwardVelocity = 0.f;
			ApplyToMesh();
			bAtRest = true;
		}
		return;
	}

	bAtRest = false;
	ApplyToMesh();
}


void UGameFeelComponent::UpdateAirState()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MoveComp) return;

	const bool bFalling = MoveComp->IsFalling();

	// 점프 순간: 위로 튀어 오를 때만 늘어남 (턱에서 걸어 내려가는 건 제외)
	if (bFalling && !bWasFalling && MoveComp->Velocity.Z > 100.f)
	{
		AddSquash(-JumpStretch);
	}

	// 착지 순간: 떨어지던 속도에 비례해서 납작해짐 + 착지 효과
	if (!bFalling && bWasFalling && MoveComp->IsMovingOnGround() && LastFallSpeed >= LandSpeedThreshold)
	{
		const float Alpha = FMath::Clamp((LastFallSpeed - LandSpeedThreshold) / FMath::Max(LandSpeedForMaxSquash - LandSpeedThreshold, 1.f), 0.f, 1.f);
		AddSquash(FMath::Lerp(LandSquashMin, LandSquashMax, Alpha));

		const FVector FeetLocation = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (LandVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LandVFX, FeetLocation);
		}
		if (LandSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, LandSound, FeetLocation);
		}
	}

	LastFallSpeed = bFalling ? FMath::Max(0.f, -MoveComp->Velocity.Z) : 0.f;
	bWasFalling = bFalling;
}


void UGameFeelComponent::UpdateLean(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;

	float TargetRight = 0.f;
	float TargetForward = 0.f;
	if (MoveComp && !MoveComp->IsFalling() && DeltaTime > KINDA_SMALL_NUMBER)
	{
		// 실제 속도 변화(가속도)를 몸 기준으로 나눠서, 가속하는 쪽으로 기울임.
		// 출발하면 앞으로, 멈추면 뒤로, 방향을 틀면 안쪽으로 기울어짐
		FVector Accel = (MoveComp->Velocity - LastVelocity) / DeltaTime;
		Accel.Z = 0.f;
		Accel = Accel.GetClampedToMaxSize(AccelForMaxLean);

		const float Scale = 1.f / FMath::Max(AccelForMaxLean, 1.f);
		TargetRight = FVector::DotProduct(Accel, Character->GetActorRightVector()) * Scale * MaxLeanRoll;
		TargetForward = FVector::DotProduct(Accel, Character->GetActorForwardVector()) * Scale * MaxLeanPitch;
	}
	LastVelocity = MoveComp ? MoveComp->Velocity : FVector::ZeroVector;

	StepSpring(LeanRight, LeanRightVelocity, TargetRight, LeanFrequency, LeanDampingRatio, DeltaTime);
	StepSpring(LeanForward, LeanForwardVelocity, TargetForward, LeanFrequency, LeanDampingRatio, DeltaTime);
}


void UGameFeelComponent::ApplyToMesh()
{
	if (!Mesh) return;

	// 키가 줄면 그만큼 옆으로 퍼지게 해서 부피를 유지
	const float Height = 1.f - FMath::Clamp(bEnableSquash ? Squash : 0.f, -MaxSquash, MaxSquash);
	const float Width = 1.f / FMath::Sqrt(Height);
	Mesh->SetRelativeScale3D(BaseScale * FVector(Width, Width, Height) * AppearScale);

	// 캡슐 기준 앞(X)축으로 좌우, 오른쪽(Y)축으로 앞뒤 기울임. 메시 원점(발밑)을 축으로 돎
	const FQuat Lean = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-LeanRight))
		* FQuat(FVector::RightVector, FMath::DegreesToRadians(LeanForward));
	Mesh->SetRelativeRotation(Lean * BaseRotation);
}


void UGameFeelComponent::AddSquash(float Amount)
{
	if (!bEnableSquash || !Mesh) return;

	// 같은 방향이면 누적, 반대 방향이면(늘어난 채 착지 등) 새 변형으로 덮어씀
	Squash = (Squash * Amount > 0.f) ? Squash + Amount : Amount;
	Squash = FMath::Clamp(Squash, -MaxSquash, MaxSquash);
	SquashVelocity = 0.f;
	bAtRest = false;
	ApplyToMesh();
}


void UGameFeelComponent::HitStop(float Duration)
{
	AActor* Owner = GetOwner();
	if (!Owner || Duration <= 0.f) return;

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (TimerManager.IsTimerActive(HitStopTimerHandle) && TimerManager.GetTimerRemaining(HitStopTimerHandle) >= Duration) return;

	// 타이머는 액터의 시간 배율과 상관없이 실제 시간으로 흐르므로, 멈춘 동안에도 정확히 Duration 뒤에 풀림
	Owner->CustomTimeDilation = HitStopTimeScale;
	TimerManager.SetTimer(HitStopTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (AActor* StoppedOwner = GetOwner())
		{
			StoppedOwner->CustomTimeDilation = 1.f;
		}
	}), Duration, false);
}


void UGameFeelComponent::PlayHitReaction(bool bHeavy, bool bKilled)
{
	AddSquash(HitSquash * (bKilled ? KillSquashMultiplier : 1.f));
	HitStop(bKilled ? HitStopKill : (bHeavy ? HitStopHeavy : HitStopLight));
}


void UGameFeelComponent::SetAppearScale(float Scale)
{
	AppearScale = Scale;
	bAtRest = false;
	ApplyToMesh();
}


void UGameFeelComponent::StepSpring(float& Value, float& Velocity, float Target, float Frequency, float DampingRatio, float DeltaTime)
{
	if (DeltaTime <= 0.f) return;

	// 프레임이 길어도 튀지 않도록 작은 간격으로 나눠서 적분
	const float Omega = 2.f * PI * FMath::Max(Frequency, 0.01f);
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(DeltaTime / 0.008f), 1, 13);
	const float Step = DeltaTime / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		const float Accel = Omega * Omega * (Target - Value) - 2.f * DampingRatio * Omega * Velocity;
		Velocity += Accel * Step;
		Value += Velocity * Step;
	}
}
