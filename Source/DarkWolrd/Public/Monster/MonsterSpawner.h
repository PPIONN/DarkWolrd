// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterSpawner.generated.h"

class ABaseMonster;
class UBillboardComponent;
class UDrawSphereComponent;
class UNiagaraSystem;
class USoundBase;

// FMonsterSpawnEntry: 이 스포너에서 나올 수 있는 몬스터 한 종류와 나올 확률 비중
USTRUCT(BlueprintType)
struct FMonsterSpawnEntry
{
	GENERATED_BODY()

	// MonsterClass: 나올 몬스터 BP (BaseMonster를 부모로 만든 BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	// Weight: 다른 종류와 비교한 등장 비중 (예: 3과 1이면 3:1 확률)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Weight = 1.f;
};

// AMonsterSpawner: 사냥터에 놓는 일반 몬스터 무리 스포너.
// 게임이 시작되면 SpawnRadius 안의 무작위 지점에 MaxAlive마리를 채우고,
// 한 마리가 쓰러질 때마다 RespawnDelay초 뒤 다시 채움 → 계속 사냥할 수 있음 (자동 사냥·잠수용).
// 나온 몬스터는 나온 자리를 "집"으로 삼아 그 주변을 순찰하고, 끌려가도 집으로 돌아옴
UCLASS()
class DARKWOLRD_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

	// MonsterTypes: 나올 몬스터 종류들. 여러 종류면 Weight 비중대로 섞여서 나옴
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner")
	TArray<FMonsterSpawnEntry> MonsterTypes;

	// MaxAlive: 동시에 살아 있을 최대 마리 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner", meta = (ClampMin = "0"))
	int32 MaxAlive = 4;

	// SpawnRadius: 스포너를 중심으로 이 반경 안에 나옴 (에디터에서 노란 구로 보임)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner", meta = (ClampMin = "0"))
	float SpawnRadius = 600.f;

	// RespawnDelay: 한 마리가 쓰러진 뒤 다시 나오기까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner", meta = (ClampMin = "0"))
	float RespawnDelay = 10.f;

	// MinLevel / MaxLevel: 나오는 몬스터의 레벨 범위. 0이면 몬스터 BP에 적힌 레벨 그대로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner", meta = (ClampMin = "0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner", meta = (ClampMin = "0"))
	int32 MaxLevel = 0;

	// bActive: 끄면 새로 스폰하지 않음 (이미 나온 몬스터는 그대로)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner")
	bool bActive = true;

	// ─── 등장 연출 (선택) ───────────────────────────────────────

	// AppearDuration: 몸이 작게 시작해서 튀어나오듯 커지는 시간(초). 0이면 바로 원래 크기로 나옴
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner|FX", meta = (ClampMin = "0"))
	float AppearDuration = 0.35f;

	// SpawnVFX / SpawnSound: 나오는 순간 발밑에 재생. 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner|FX")
	UNiagaraSystem* SpawnVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Spawner|FX")
	USoundBase* SpawnSound = nullptr;

	// SpawnOne: 지금 바로 한 마리 스폰 (MaxAlive와 상관없이). 실패하면 nullptr
	UFUNCTION(BlueprintCallable, Category = "Monster Spawner")
	ABaseMonster* SpawnOne();

	// GetAliveCount: 지금 살아 있는 이 스포너 몬스터 수
	UFUNCTION(BlueprintCallable, Category = "Monster Spawner")
	int32 GetAliveCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// HandleMonsterDied: 목록에서 빼고 RespawnDelay 뒤 다시 채우도록 예약
	UFUNCTION()
	void HandleMonsterDied(ABaseMonster* DeadMonster);

	// CheckRespawn: 주기적으로 호출. 예약 시간이 지난 자리를 MaxAlive까지 채움
	void CheckRespawn();

	// PickMonsterClass: Weight 비중대로 한 종류를 고름. 고를 게 없으면 nullptr
	UClass* PickMonsterClass() const;

	// FindSpawnLocation: 반경 안 무작위 지점에서 바닥(가능하면 내비메시 위)을 찾아 캡슐 높이만큼 띄운 위치
	bool FindSpawnLocation(float CapsuleHalfHeight, FVector& OutLocation) const;

	UPROPERTY(VisibleAnywhere, Category = "Monster Spawner")
	UBillboardComponent* Billboard;

	// RadiusPreview: 에디터에서만 보이는 스폰 반경 표시
	UPROPERTY(VisibleAnywhere, Category = "Monster Spawner")
	UDrawSphereComponent* RadiusPreview;

	TArray<TWeakObjectPtr<ABaseMonster>> AliveMonsters;

	// RespawnTimes: 다시 채울 예정 시각들 (월드 시간). 하나가 한 마리 몫
	TArray<float> RespawnTimes;

	FTimerHandle CheckTimerHandle;
};
