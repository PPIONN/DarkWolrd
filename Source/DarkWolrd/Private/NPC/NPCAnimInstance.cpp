// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/NPCAnimInstance.h"
#include "NPC/NPCBase.h"


// 매 프레임: 주인 NPC의 대화 상태와 쳐다볼 위치를 읽고, 머리 돌리기 강도를 부드럽게 따라가게 함
void UNPCAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ANPCBase* NPC = Cast<ANPCBase>(TryGetPawnOwner());
	if (!NPC)
	{
		bIsTalking = false;
		LookAtAlpha = 0.f;
		return;
	}

	bIsTalking = NPC->IsTalking();

	FVector Target;
	const bool bHasTarget = NPC->GetLookAtLocation(Target);
	if (bHasTarget)
	{
		LookAtLocation = Target;
	}

	LookAtAlpha = FMath::FInterpTo(LookAtAlpha, bHasTarget ? LookAtMaxAlpha : 0.f, DeltaSeconds, LookAtBlendSpeed);
}
