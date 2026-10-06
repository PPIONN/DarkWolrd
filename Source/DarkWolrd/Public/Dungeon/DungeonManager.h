// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonManager.generated.h"

class ADungeonRoom;
class ADungeonPortal;
class ABaseMonster;
class UBillboardComponent;

// FDungeonCardReward: 던전 클리어 시 Chance 확률로 보유 목록에 들어오는 몬스터 카드
USTRUCT(BlueprintType)
struct FDungeonCardReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 1.f;
};

// ADungeonManager: 던전 레벨에 하나 배치하는 진행 관리자.
// 입장 알림을 띄우고, 첫 방이 시작될 때 시간을 재기 시작하고, FinalRoom을 클리어하면
// 클리어 시간 알림 + 보상(경험치·돈·카드) 지급 + ExitPortal 켜기를 함
UCLASS()
class DARKWOLRD_API ADungeonManager : public AActor
{
	GENERATED_BODY()

public:
	ADungeonManager();

	// DungeonName: 알림에 쓸 던전 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon")
	FText DungeonName;

	// FinalRoom: 이 방을 클리어하면 던전 클리어 (보통 보스 방)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dungeon")
	ADungeonRoom* FinalRoom = nullptr;

	// ExitPortal: 클리어하면 켜질 출구 포털 (bStartsActive를 꺼둔 포털)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dungeon")
	ADungeonPortal* ExitPortal = nullptr;

	// ClearRewardEXP / ClearRewardMoney / RewardCards: 클리어 보상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Reward", meta = (ClampMin = "0"))
	int32 ClearRewardEXP = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Reward", meta = (ClampMin = "0"))
	int64 ClearRewardMoney = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Reward")
	TArray<FDungeonCardReward> RewardCards;

	// EnterAnnouncement / ClearAnnouncement: {Dungeon}은 던전 이름, {Time}은 클리어 시간(분:초), {Reward}는 보상 요약으로 바뀜
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Announcement")
	FText EnterAnnouncement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Announcement")
	FText ClearAnnouncement;

	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	bool IsDungeonCleared() const { return bCleared; }

	// GetElapsedTime: 첫 방 시작부터 지금(클리어했으면 클리어 순간)까지의 시간(초). 시작 전이면 0
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	float GetElapsedTime() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleRoomStarted(ADungeonRoom* Room);

	UFUNCTION()
	void HandleFinalRoomCleared(ADungeonRoom* Room);

	UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	UBillboardComponent* Billboard;

	bool bCleared = false;
	float StartTime = -1.f;
	float ClearTime = -1.f;
};
