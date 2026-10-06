// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FieldBossSpawner.generated.h"

class AFieldBossMonster;
class ABaseMonster;
class UBillboardComponent;

// FFieldBossSpawnPoint: 보스가 나타날 수 있는 후보 지점 하나
USTRUCT(BlueprintType)
struct FFieldBossSpawnPoint
{
	GENERATED_BODY()

	// Point: 레벨에 배치한 Target Point 같은 위치 표시용 액터. 보스가 이 위치(바닥 기준)에 나타남
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	AActor* Point = nullptr;

	// LocationName: 알림에 쓸 지역 이름 (예: "검은 숲 북쪽")
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText LocationName;
};

// AFieldBossSpawner: 필드보스 한 종류를 관리하는 배치용 액터.
// 게임 시작 후 FirstSpawnDelay가 지나면 후보 지점 중 한 곳에 보스를 소환하고 화면에 알림을 띄움.
// 보스가 쓰러지면 처치 알림 후 RespawnDelay 뒤에 (가능하면 직전과 다른) 후보 지점에 다시 나타남.
// 한 스포너당 동시에 한 마리만 존재함
UCLASS()
class DARKWOLRD_API AFieldBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	AFieldBossSpawner();

	// BossClass: 소환할 필드보스 BP (FieldBossMonster를 부모로 만든 BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	TSubclassOf<AFieldBossMonster> BossClass;

	// SpawnPoints: 후보 지점 목록. 비워두면 이 스포너가 놓인 자리에 나타남
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	TArray<FFieldBossSpawnPoint> SpawnPoints;

	// FirstSpawnDelay: 게임 시작 후 첫 등장까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss", meta = (ClampMin = "0"))
	float FirstSpawnDelay = 60.f;

	// RespawnDelay: 보스가 쓰러진 뒤 다시 나타나기까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss", meta = (ClampMin = "0"))
	float RespawnDelay = 600.f;

	// SpawnAnnouncement / DefeatAnnouncement: 등장·처치 알림 문구. {Boss}는 보스 이름, {Location}은 지역 이름으로 바뀜
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss|Announcement")
	FText SpawnAnnouncement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss|Announcement")
	FText DefeatAnnouncement;

	// AnnouncementDuration: 알림이 화면에 떠 있는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss|Announcement")
	float AnnouncementDuration = 6.f;

	// SpawnBossNow: 기다리지 않고 지금 바로 소환 (이미 살아 있으면 그 보스를 반환). 테스트·이벤트용
	UFUNCTION(BlueprintCallable, Category = "Field Boss")
	AFieldBossMonster* SpawnBossNow();

	// GetCurrentBoss: 지금 살아 있는 보스 (없으면 nullptr)
	UFUNCTION(BlueprintCallable, Category = "Field Boss")
	AFieldBossMonster* GetCurrentBoss() const { return CurrentBoss; }

	// GetTimeUntilNextSpawn: 다음 등장까지 남은 시간(초). 보스가 살아 있거나 예약이 없으면 -1
	UFUNCTION(BlueprintCallable, Category = "Field Boss")
	float GetTimeUntilNextSpawn() const;

protected:
	virtual void BeginPlay() override;

private:
	// HandleBossDied: 보스 사망 시 처치 알림을 띄우고 다음 등장을 예약
	UFUNCTION()
	void HandleBossDied(ABaseMonster* DeadMonster);

	// PickSpawnPointIndex: 후보 중 하나를 무작위로 고름 (후보가 2곳 이상이면 직전 지점은 피함). 후보가 없으면 -1
	int32 PickSpawnPointIndex() const;

	void SpawnFromTimer();

	UPROPERTY(VisibleAnywhere, Category = "Field Boss")
	UBillboardComponent* Billboard;

	UPROPERTY()
	AFieldBossMonster* CurrentBoss = nullptr;

	int32 LastSpawnPointIndex = INDEX_NONE;

	FText CurrentLocationName;

	FTimerHandle SpawnTimerHandle;
};
