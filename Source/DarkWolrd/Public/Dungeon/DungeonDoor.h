// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonDoor.generated.h"

class UStaticMeshComponent;

// ADungeonDoor: 던전 방 사이의 문. 열리면 문 메시가 OpenOffset만큼(기본: 바닥 아래로) 부드럽게 움직이고, 닫히면 원래 자리로 돌아와 길을 막음.
// ADungeonRoom이 전투 시작 때 닫고 클리어하면 연다. 레벨에 배치한 뒤 DoorMesh에 문 모양 메시를 지정하면 됨
UCLASS()
class DARKWOLRD_API ADungeonDoor : public AActor
{
	GENERATED_BODY()

public:
	ADungeonDoor();

	// DoorMesh: 문 모양 (에디터에서 메시 지정). 닫혀 있을 때의 위치 기준으로 배치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon Door")
	UStaticMeshComponent* DoorMesh;

	// bStartsOpen: 게임 시작 시 열려 있을지. 방 입구는 열림, 다음 방으로 가는 출구는 닫힘으로 두면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Door")
	bool bStartsOpen = true;

	// OpenOffset: 열렸을 때 문 메시가 닫힌 자리에서 얼마나 이동해 있을지 (기본: 4m 아래로 내려가 바닥에 숨음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Door")
	FVector OpenOffset = FVector(0.f, 0.f, -400.f);

	// MoveSpeed: 문이 움직이는 속도 (cm/초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Door", meta = (ClampMin = "1"))
	float MoveSpeed = 600.f;

	// SetOpen: 문을 열거나 닫음 (움직임은 Tick에서 부드럽게 진행)
	UFUNCTION(BlueprintCallable, Category = "Dungeon Door")
	void SetOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Dungeon Door")
	bool IsOpen() const { return bIsOpen; }

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsOpen = true;

	// ClosedLocation: 닫힌 상태의 문 메시 상대 위치 (배치한 그대로)
	FVector ClosedLocation = FVector::ZeroVector;
};
