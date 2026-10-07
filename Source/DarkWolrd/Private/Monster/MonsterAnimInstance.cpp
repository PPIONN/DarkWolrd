// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/MonsterAnimInstance.h"
#include "GameFramework/Pawn.h"

// 매 프레임: 이 애니메이션을 쓰는 폰의 수평 속도를 읽음. 폰이 없으면(에디터 미리보기 등) 정지 상태로 둠
void UMonsterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* OwnerPawn = TryGetPawnOwner();
	Speed = OwnerPawn ? OwnerPawn->GetVelocity().Size2D() : 0.f;
	bIsMoving = Speed > MovingSpeedThreshold;
}
