// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/RabbitAnimInstance.h"
#include "Character/Rabbit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"

void URabbitAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Rabbit = Cast<ARabbit>(TryGetPawnOwner());
	NextBoredIdleTime = BoredIdleDelay;
}


void URabbitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 미리보기 등 토끼가 아닌 곳에서 돌 때는 기본값 그대로 둠
	ARabbit* Owner = Rabbit.Get();
	if (!Owner)
	{
		Rabbit = Cast<ARabbit>(TryGetPawnOwner());
		return;
	}

	const UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement();
	const FVector Velocity = Owner->GetVelocity();

	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;
	bIsFalling = MoveComp && MoveComp->IsFalling();
	bShouldMove = GroundSpeed > MoveThreshold && MoveComp && !MoveComp->GetCurrentAcceleration().IsNearlyZero();

	bIsAiming = Owner->IsAiming();
	bIsStrafing = Owner->IsStrafing();
	Direction = bIsStrafing ? Owner->MoveInputAngle : 0.f;

	// 바라볼 지점이 몸 정면에서 많이 벗어날수록(뒤쪽일수록) Look At을 약하게
	LookAtTarget = Owner->HeadLookAtTarget;
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget = (LookAtTarget - Owner->GetActorLocation()).GetSafeNormal2D();
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f)));
	const float TargetAlpha = 1.f - FMath::Clamp((Angle - LookAtFullAngle) / FMath::Max(LookAtZeroAngle - LookAtFullAngle, 1.f), 0.f, 1.f);
	LookAtAlpha = FMath::FInterpTo(LookAtAlpha, TargetAlpha, DeltaSeconds, LookAtBlendSpeed);

	UpdateBoredIdle();
}


void URabbitAnimInstance::UpdateBoredIdle()
{
	if (!BoredIdleMontage) return;

	const float TimeSinceInput = Rabbit->GetTimeSinceLastInput();

	// 입력이 들어왔거나(시간이 0으로 리셋됨) 움직이는 중이면: 재생 중인 몽타주를 멈추고 카운트다운을 다시 시작
	if (TimeSinceInput < LastTimeSinceInput || bShouldMove || bIsFalling)
	{
		NextBoredIdleTime = TimeSinceInput + BoredIdleDelay;
		if (Montage_IsPlaying(BoredIdleMontage))
		{
			Montage_Stop(0.25f, BoredIdleMontage);
		}
	}
	// 충분히 오래 가만히 있었고, 공격 등 다른 몽타주가 재생 중이 아니면 재생
	else if (TimeSinceInput >= NextBoredIdleTime && !IsAnyMontagePlaying())
	{
		Montage_Play(BoredIdleMontage);
		NextBoredIdleTime = TimeSinceInput + BoredIdleMontage->GetPlayLength() + BoredIdleRepeatDelay;
	}

	LastTimeSinceInput = TimeSinceInput;
}
