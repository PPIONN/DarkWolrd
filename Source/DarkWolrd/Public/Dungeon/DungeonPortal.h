// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonPortal.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ARabbit;

// ADungeonPortal: 밟으면 다른 레벨로 이동하는 포털. 던전 입구(필드 → 던전)와 출구(던전 → 원래 자리)로 씀.
//   - 입구: TargetLevel에 던전 레벨 지정. 들어갈 때 "이 포털 앞"을 돌아올 위치로 기억함
//   - 출구: bReturnToPreviousLevel 켜기. 들어왔던 레벨의 입구 포털 앞으로 돌아감. 보통 bStartsActive를 끄고
//           ADungeonManager가 던전 클리어 시 켜줌
// 이동할 때 플레이어 HP·스탯·카드는 그대로 따라감 (소환수는 새 레벨에서 Y로 다시 소환)
UCLASS()
class DARKWOLRD_API ADungeonPortal : public AActor
{
	GENERATED_BODY()

public:
	ADungeonPortal();

	// Trigger: 플레이어가 닿으면 이동하는 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	USphereComponent* Trigger;

	// PortalMesh: 포털 모양 (에디터에서 메시 지정, 없어도 동작)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UStaticMeshComponent* PortalMesh;

	// TargetLevel: 이동할 레벨 (입구 포털용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	TSoftObjectPtr<UWorld> TargetLevel;

	// bReturnToPreviousLevel: 켜면 TargetLevel 대신 들어오기 전 레벨의 입구 포털 앞으로 돌아감 (출구 포털용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	bool bReturnToPreviousLevel = false;

	// bStartsActive: 게임 시작 시 켜져 있을지. 꺼진 포털은 보이지 않고 밟아도 반응 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	bool bStartsActive = true;

	// TravelDelay: 밟은 뒤 실제로 이동하기까지의 시간(초). 그동안 이동 알림이 표시됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal", meta = (ClampMin = "0"))
	float TravelDelay = 1.f;

	// TravelMessage: 이동할 때 띄울 알림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FText TravelMessage;

	// ReturnOffset: 던전에서 돌아왔을 때 이 포털 기준으로 서는 위치 (기본: 포털 앞 2.5m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FVector ReturnOffset = FVector(250.f, 0.f, 0.f);

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPortalActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Portal")
	bool IsPortalActive() const { return bIsActive; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Travel: 플레이어 상태를 넘길 준비를 하고 레벨을 엶
	void Travel();

	bool bIsActive = true;
	bool bTraveling = false;

	TWeakObjectPtr<ARabbit> TravelingPlayer;

	FTimerHandle TravelTimerHandle;
};
