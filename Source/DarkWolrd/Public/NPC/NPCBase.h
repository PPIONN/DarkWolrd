// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCBase.generated.h"

// ANPCBase: 대화 가능한 NPC의 공용 베이스 클래스. 새 NPC를 만들 때 이 클래스를 상속받아
// NPCDisplayName과 DialogueLines만 에디터에서 채우면 됨
UCLASS()
class DARKWOLRD_API ANPCBase : public ACharacter
{
	GENERATED_BODY()

public:
	ANPCBase();

	// NPCDisplayName: 대화창에 표시할 이 NPC의 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText NPCDisplayName;

	// DialogueLines: 순서대로 보여줄 대사 목록. F를 누를 때마다 한 줄씩 진행됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TArray<FText> DialogueLines;

	// GetNextDialogueLine: 현재 인덱스의 대사를 반환하고 다음으로 넘김.
	// 마지막 대사를 반환하는 거였다면 bOutIsLastLine을 true로 설정하고, 다음 대화는 다시 처음부터 시작하도록 인덱스를 리셋
	UFUNCTION(BlueprintCallable, Category = "NPC")
	FText GetNextDialogueLine(bool& bOutIsLastLine);

	// FacingReturnDistance: 대화 후 플레이어가 이 거리보다 멀어지면 원래 바라보던 방향으로 되돌아감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Facing")
	float FacingReturnDistance = 600.f;

	// FaceTurnSpeed: 플레이어 쪽으로/원래 방향으로 돌아갈 때의 회전 속도 (작을수록 천천히 부드럽게 돎)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Facing")
	float FaceTurnSpeed = 5.f;

	// StartFacingPlayer: 대화가 시작(또는 진행)될 때 호출됨. 이후 플레이어가 FacingReturnDistance 안에 있는 동안
	// 계속 그쪽을 바라보고, 멀어지면 자동으로 원래 방향으로 복귀함
	UFUNCTION(BlueprintCallable, Category = "NPC|Facing")
	void StartFacingPlayer(AActor* Player);

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	// CurrentDialogueIndex: 다음에 보여줄 대사의 인덱스
	int32 CurrentDialogueIndex = 0;

	// DefaultRotation: 게임 시작 시 이 NPC가 원래 바라보고 있던 방향 (BeginPlay에서 저장)
	FRotator DefaultRotation;

	// bIsFacingPlayer: 지금 플레이어 쪽을 바라보는 중인지. 거리가 멀어지면 자동으로 false가 되어 원래 방향으로 복귀
	bool bIsFacingPlayer = false;

	// TrackedPlayer: 지금 바라보고 있는 대상(플레이어)
	UPROPERTY()
	AActor* TrackedPlayer;
};
