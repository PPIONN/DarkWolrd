// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RabbitAnimInstance.generated.h"

class ARabbit;
class UAnimMontage;

// URabbitAnimInstance: 토끼 애님 블루프린트의 부모 클래스.
// 애님 그래프에서 쓰는 값(속도, 방향, 공중 여부, 조준, 머리 Look At)을 매 프레임 C++에서 계산해 두므로
// 애님 블루프린트의 이벤트 그래프는 비워 두고 애님 그래프만 만들면 된다.
// 오래 가만히 있을 때 재생하는 지루한 대기 몽타주(BoredIdleMontage)도 여기서 재생·중지한다.
UCLASS()
class DARKWOLRD_API URabbitAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// GroundSpeed: 수평 이동 속도 (블렌드 스페이스 GroundSpeed 축)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	float GroundSpeed = 0.f;

	// Direction: 블렌드 스페이스 Direction 축(-180~180, 몸 정면 기준 0=정면).
	// 탐험 상태에서는 몸이 달리는 방향을 보므로 항상 0, 조준 중·공격 직후에는 이동 방향(옆걸음·뒷걸음)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	float Direction = 0.f;

	// bShouldMove: 속도도 있고 이동 입력도 있는 상태. Idle ↔ 이동 전환에 사용
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	bool bShouldMove = false;

	// bIsFalling: 공중에 있는지 (점프·낙하 모두)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	bool bIsFalling = false;

	// VerticalSpeed: 위아래 속도. 양수면 올라가는 중 (점프로 뜬 건지, 그냥 떨어지는 건지 구분)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	float VerticalSpeed = 0.f;

	// bIsAiming: 조준 키를 누르고 있는지
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	bool bIsAiming = false;

	// bIsStrafing: 몸 방향이 고정된 채 옆걸음·뒷걸음하는 상태인지 (조준 중·공격 직후)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	bool bIsStrafing = false;

	// LookAtTarget: 머리가 바라볼 월드 좌표 (카메라가 보는 먼 지점)
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	FVector LookAtTarget = FVector::ZeroVector;

	// LookAtAlpha: Look At 노드의 Alpha. 바라볼 지점이 몸 뒤쪽이면 0으로 줄어들어 고개가 뒤로 꺾이지 않음
	UPROPERTY(BlueprintReadOnly, Category = "Rabbit")
	float LookAtAlpha = 0.f;

	// MoveThreshold: 이 속도보다 빨라야 움직이는 것으로 봄
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Settings")
	float MoveThreshold = 3.f;

	// LookAtFullAngle / LookAtZeroAngle: 몸 정면과 바라볼 지점 사이 각도가 Full 이하면 Alpha 1, Zero 이상이면 0
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Settings")
	float LookAtFullAngle = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Settings")
	float LookAtZeroAngle = 110.f;

	// LookAtBlendSpeed: LookAtAlpha가 목표값으로 따라가는 속도
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Settings")
	float LookAtBlendSpeed = 5.f;

	// BoredIdleMontage: 입력 없이 BoredIdleDelay초 동안 가만히 있으면 재생할 몽타주 (DefaultSlot). 비워두면 사용 안 함
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Bored Idle")
	UAnimMontage* BoredIdleMontage = nullptr;

	// BoredIdleDelay: 마지막 입력 후 몇 초가 지나면 지루한 대기 몽타주를 재생할지
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Bored Idle")
	float BoredIdleDelay = 10.f;

	// BoredIdleRepeatDelay: 계속 가만히 있으면, 몽타주가 끝난 뒤 몇 초 뒤에 다시 재생할지
	UPROPERTY(EditDefaultsOnly, Category = "Rabbit|Bored Idle")
	float BoredIdleRepeatDelay = 10.f;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	// UpdateBoredIdle: 입력이 오래 없으면 지루한 대기 몽타주를 재생하고, 입력이 들어오면 멈춤
	void UpdateBoredIdle();

	TWeakObjectPtr<ARabbit> Rabbit;

	// NextBoredIdleTime: TimeSinceLastInput이 이 값에 도달하면 지루한 대기 몽타주 재생
	float NextBoredIdleTime = 0.f;
	float LastTimeSinceInput = 0.f;
};
