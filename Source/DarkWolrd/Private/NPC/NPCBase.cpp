// Fill out your copyright notice in the Description page of Project Settings.

#include "NPC/NPCBase.h"

ANPCBase::ANPCBase()
{
	PrimaryActorTick.bCanEverTick = true;
}


// 게임 시작 시: 원래 바라보던 방향을 저장해둠 (나중에 대화 끝나고 플레이어가 멀어지면 이 방향으로 복귀)
void ANPCBase::BeginPlay()
{
	Super::BeginPlay();

	DefaultRotation = GetActorRotation();
}


// 다음 대사 반환: 현재 인덱스의 대사를 돌려주고 인덱스를 하나 증가.
// 이번에 반환한 게 마지막 대사였으면 bOutIsLastLine=true로 표시하고, 다음 대화를 위해 인덱스를 0으로 리셋
FText ANPCBase::GetNextDialogueLine(bool& bOutIsLastLine)
{
	bOutIsLastLine = false;

	if (DialogueLines.Num() == 0)
	{
		bOutIsLastLine = true;
		return FText::GetEmpty();
	}

	const FText Line = DialogueLines[CurrentDialogueIndex];
	++CurrentDialogueIndex;

	if (CurrentDialogueIndex >= DialogueLines.Num())
	{
		CurrentDialogueIndex = 0;
		bOutIsLastLine = true;
	}

	return Line;
}


// 대화 시작(또는 진행) 시 호출: 이 대상을 추적 대상으로 설정하고 추적 시작
void ANPCBase::StartFacingPlayer(AActor* Player)
{
	if (!Player) return;

	TrackedPlayer = Player;
	bIsFacingPlayer = true;
}


// 매 프레임: 추적 중이면 플레이어 쪽을, 아니면(또는 너무 멀어졌으면) 원래 방향을 목표로 삼아 부드럽게 회전
void ANPCBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFacingPlayer && IsValid(TrackedPlayer))
	{
		const float Distance = FVector::Dist(GetActorLocation(), TrackedPlayer->GetActorLocation());
		if (Distance > FacingReturnDistance)
		{
			// 너무 멀어지면 추적을 그만두고 원래 방향으로 복귀
			bIsFacingPlayer = false;
		}
	}
	else
	{
		bIsFacingPlayer = false;
	}

	FRotator TargetRotation = DefaultRotation;
	if (bIsFacingPlayer && IsValid(TrackedPlayer))
	{
		const FVector ToPlayer = (TrackedPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToPlayer.IsNearlyZero())
		{
			TargetRotation = ToPlayer.Rotation();
		}
	}

	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, FaceTurnSpeed));
}
