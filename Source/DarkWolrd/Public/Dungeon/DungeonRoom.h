// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonRoom.generated.h"

class UBoxComponent;
class ABaseMonster;
class ADungeonDoor;
class ARabbit;
class ADungeonRoom;

// FDungeonSpawnEntry: 웨이브에 나올 몬스터 한 종류
USTRUCT(BlueprintType)
struct FDungeonSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Level = 1;
};

// FDungeonWave: 한 번에 나오는 몬스터 묶음. 앞 웨이브를 전부 잡으면 StartDelay 뒤에 나옴
USTRUCT(BlueprintType)
struct FDungeonWave
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDungeonSpawnEntry> Monsters;

	// StartDelay: 방 시작(첫 웨이브) 또는 앞 웨이브 처치 후 이 웨이브가 나오기까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float StartDelay = 1.5f;

	// Announcement: 이 웨이브가 나올 때 띄울 알림 (비우면 알림 없음). 마지막 방 보스 등장 알림 등에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Announcement;
};

UENUM(BlueprintType)
enum class EDungeonRoomState : uint8
{
	Waiting,	// 아직 아무도 안 들어옴
	InProgress,	// 전투 중 (문 잠김)
	Cleared		// 클리어 (문 열림)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonRoomEvent, ADungeonRoom*, Room);

// ADungeonRoom: 방 단위 던전의 방 하나.
// 플레이어가 RoomTrigger(방 안쪽을 덮는 상자) 안으로 들어오면 Doors를 전부 닫고 Waves를 차례로 소환함.
// 웨이브를 모두 잡으면 Doors를 전부 열어 다음 방으로 갈 수 있게 함.
// 전투 중 플레이어가 쓰러지면 방을 처음 상태로 되돌려서(몬스터 제거, 입구 다시 열림) 다시 도전할 수 있게 함
UCLASS()
class DARKWOLRD_API ADungeonRoom : public AActor
{
	GENERATED_BODY()

public:
	ADungeonRoom();

	// RoomTrigger: 방 안쪽을 덮도록 크기를 맞추는 상자. 문을 지나 완전히 들어왔을 때 닿도록 문보다 안쪽에 두는 게 좋음
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Room")
	UBoxComponent* RoomTrigger;

	// RoomName: 알림에 쓸 방 이름 (예: "지하 감옥")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Room")
	FText RoomName;

	// Waves: 차례로 나올 몬스터 웨이브. 비워두면 들어오는 순간 바로 클리어(통로용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Room")
	TArray<FDungeonWave> Waves;

	// SpawnPoints: 몬스터가 나타날 위치들(Target Point 등). 비워두면 방 상자 안의 갈 수 있는 곳에서 무작위로 고름
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dungeon Room")
	TArray<AActor*> SpawnPoints;

	// Doors: 전투가 시작되면 닫히고 클리어하면 열리는 문들 (이 방의 입구와 출구를 모두 넣음)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dungeon Room")
	TArray<ADungeonDoor*> Doors;

	// RequiredRoom: 이 방이 먼저 클리어돼야 이 방이 시작됨 (비우면 조건 없음)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dungeon Room")
	ADungeonRoom* RequiredRoom = nullptr;

	// MonsterAggroRadius: 방 몬스터의 감지 범위. 방 전체를 덮을 만큼 크게 둬서 나오자마자 덤벼들게 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Room")
	float MonsterAggroRadius = 3000.f;

	// ClearAnnouncement: 클리어 알림. {Room}은 방 이름으로 바뀜 (비우면 알림 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Room")
	FText ClearAnnouncement;

	UPROPERTY(BlueprintAssignable, Category = "Dungeon Room")
	FOnDungeonRoomEvent OnRoomStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dungeon Room")
	FOnDungeonRoomEvent OnRoomCleared;

	// StartRoom: 문을 닫고 첫 웨이브를 예약 (보통 플레이어가 들어올 때 자동 호출)
	UFUNCTION(BlueprintCallable, Category = "Dungeon Room")
	void StartRoom(ARabbit* InChallenger);

	// ResetRoom: 전투 중이던 방을 처음 상태로 되돌림 (남은 몬스터 제거, 시작 전에 열려 있던 문 다시 열기)
	UFUNCTION(BlueprintCallable, Category = "Dungeon Room")
	void ResetRoom();

	UFUNCTION(BlueprintCallable, Category = "Dungeon Room")
	EDungeonRoomState GetRoomState() const { return RoomState; }

	// GetCurrentWaveNumber: 지금 몇 번째 웨이브인지 (1부터, 시작 전이면 0)
	UFUNCTION(BlueprintCallable, Category = "Dungeon Room")
	int32 GetCurrentWaveNumber() const { return CurrentWaveIndex + 1; }

	UFUNCTION(BlueprintCallable, Category = "Dungeon Room")
	int32 GetAliveMonsterCount() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleMonsterDied(ABaseMonster* DeadMonster);

	UFUNCTION()
	void HandleChallengerDied();

	// SpawnWave: 웨이브 몬스터를 스폰해서 방 전투용으로 설정(넓은 감지, 귀환 안 함)
	void SpawnWave(int32 WaveIndex);

	// ScheduleNextWave: 다음 웨이브를 StartDelay 뒤로 예약, 남은 웨이브가 없으면 클리어
	void ScheduleNextWave();

	void SpawnScheduledWave();

	void ClearRoom();

	// PickSpawnLocation: SpawnPoints를 돌아가며 쓰거나, 없으면 방 상자 안의 갈 수 있는 곳을 무작위로 고름
	FVector PickSpawnLocation(int32 SpawnIndex) const;

	// GatherSummonsToChallenger: 문이 닫힐 때 방 밖에 남은 소환수를 플레이어 곁으로 불러들임
	void GatherSummonsToChallenger();

	// UnbindChallenger: 플레이어 사망 이벤트 구독 해제
	void UnbindChallenger();

	EDungeonRoomState RoomState = EDungeonRoomState::Waiting;
	int32 CurrentWaveIndex = INDEX_NONE;

	UPROPERTY()
	TArray<TWeakObjectPtr<ABaseMonster>> AliveMonsters;

	TWeakObjectPtr<ARabbit> Challenger;

	// DoorsOpenBeforeStart: 시작 직전에 열려 있던 문 (초기화할 때 이것만 다시 염)
	TArray<TWeakObjectPtr<ADungeonDoor>> DoorsOpenBeforeStart;

	FTimerHandle WaveTimerHandle;
};
