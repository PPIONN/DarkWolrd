// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NPCAnimInstance.generated.h"

// UNPCAnimInstance: NPC 애니메이션 블루프린트의 부모 클래스.
// 매 프레임 주인 NPC(ANPCBase)에게서 대화 중인지, 어디를 쳐다볼지를 읽어 AnimGraph에서 쓰기 좋게 담아둔다.
// AnimGraph: bIsTalking으로 평소 대기 ↔ 반가운 대기를 고르고, LookAtLocation/LookAtAlpha로 머리를 플레이어 쪽으로 돌림
UCLASS()
class DARKWOLRD_API UNPCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// bIsTalking: 플레이어와 대화 중인지
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	bool bIsTalking = false;

	// LookAtLocation: 머리가 향할 월드 위치. 대화가 끝나도 마지막 위치를 유지해서 알파가 줄어드는 동안 머리가 튀지 않게 함
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	FVector LookAtLocation = FVector::ZeroVector;

	// LookAtAlpha: 머리 돌리기 강도 0~1. 대화가 시작되면 서서히 1로, 끝나면 서서히 0으로
	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	float LookAtAlpha = 0.f;

	// LookAtBlendSpeed: LookAtAlpha가 따라가는 속도 (클수록 빨리 쳐다봄)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	float LookAtBlendSpeed = 4.f;

	// LookAtMaxAlpha: 대화 중 머리 돌리기 최대 강도. 1이면 머리가 플레이어에 고정돼 애니메이션의 끄덕임·갸웃이 사라지므로 조금 낮춤
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LookAtMaxAlpha = 0.8f;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
