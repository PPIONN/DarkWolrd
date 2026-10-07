// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MonsterAnimInstance.generated.h"

// UMonsterAnimInstance: 몬스터 애니메이션 BP의 공통 부모.
// 매 프레임 몸의 수평 이동속도를 계산해서 Speed / bIsMoving에 넣어두므로, 이 클래스를 부모로 고른 ABP는
// EventGraph를 만들 필요 없이 AnimGraph에서 두 값을 바로 읽어 대기/이동 애니메이션을 고르면 됨.
// (애니메이션 BP는 스켈레톤마다 따로 만들어야 하지만, 이 계산은 몬스터 종류와 무관해서 전부 공유)
UCLASS()
class DARKWOLRD_API UMonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// Speed: 몸의 수평 이동속도 (cm/초)
	UPROPERTY(BlueprintReadOnly, Category = "Monster Anim")
	float Speed = 0.f;

	// bIsMoving: Speed가 MovingSpeedThreshold보다 크면 true (이동 애니메이션으로 바꾸는 기준)
	UPROPERTY(BlueprintReadOnly, Category = "Monster Anim")
	bool bIsMoving = false;

	// MovingSpeedThreshold: 이 속도를 넘으면 움직이는 중으로 봄. 순찰·배회는 원래 속도의 절반쯤이라 낮게 둠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster Anim")
	float MovingSpeedThreshold = 10.f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
