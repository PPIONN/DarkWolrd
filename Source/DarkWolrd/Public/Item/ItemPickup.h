// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UItemDefinition;

// AItemPickup: 몬스터가 떨어뜨린 아이템. 희귀도 색으로 빛나는 작은 구슬이 바닥 위에 떠서 돌고,
// 토끼가 가까이 오면 끌려와서 닿는 순간 인벤토리에 들어감. 인벤토리가 가득 차면 못 넣은 만큼 그대로 남음
UCLASS()
class DARKWOLRD_API AItemPickup : public AActor
{
	GENERATED_BODY()

public:
	AItemPickup();

	// SpawnDrop: 아이템 Count개를 Location 근처에 떨어뜨림 (바닥을 찾아 그 위에 띄움)
	static AItemPickup* SpawnDrop(UWorld* World, UItemDefinition* Item, int32 Count, const FVector& Location);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UItemDefinition* Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 1;

	// HoverHeight: 바닥에서 떠 있는 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float HoverHeight = 35.f;

	// MagnetRadius / MagnetSpeed: 토끼가 이 거리 안에 오면 이 속도로 끌려감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float MagnetRadius = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float MagnetSpeed = 700.f;

	// PickupDelay: 떨어진 직후 이 시간(초) 동안은 끌려가지 않음 (떨어지는 모습이 보이게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float PickupDelay = 0.5f;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// TryGiveTo: 그 액터의 인벤토리에 넣어봄. 다 들어가면 사라짐
	void TryGiveTo(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, Category = "Item")
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UStaticMeshComponent* GemMesh;

	UPROPERTY(VisibleAnywhere, Category = "Item")
	UPointLightComponent* GlowLight;

private:
	FVector BaseLocation = FVector::ZeroVector;
	float Age = 0.f;
	bool bFullWarned = false;
};
