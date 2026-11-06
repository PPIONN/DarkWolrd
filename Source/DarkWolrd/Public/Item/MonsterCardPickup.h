// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/MonsterCardData.h"
#include "MonsterCardPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class UPointLightComponent;

// AMonsterCardPickup: 몬스터를 쓰러뜨렸을 때 필드에 떨어지는 몬스터 카드.
// 플레이어(정확히는 UMonsterCompanionComponent를 가진 액터)가 닿으면 자동으로 주워지고 사라짐
UCLASS()
class DARKWOLRD_API AMonsterCardPickup : public AActor
{
	GENERATED_BODY()

public:
	AMonsterCardPickup();

	// CardData: 이 카드가 담고 있는 몬스터 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	FMonsterCardEntry CardData;

protected:
	virtual void BeginPlay() override;

	// OnPickupSphereBeginOverlap: 습득 판정. UMonsterCompanionComponent가 있는 액터면 카드를 넘겨주고 파괴됨
	UFUNCTION()
	void OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	// PickupSphere: 습득 판정용 구체 (루트 컴포넌트)
	UPROPERTY(VisibleAnywhere, Category = "Monster Card")
	USphereComponent* PickupSphere;

	// CardMesh: 카드의 외형. 기본은 얇은 판 모양, 블루프린트에서 메시/머티리얼 교체 가능
	UPROPERTY(VisibleAnywhere, Category = "Monster Card")
	UStaticMeshComponent* CardMesh;

	// RotatingMovement: 카드를 제자리에서 회전시킴 (RotationRate로 속도 조절)
	UPROPERTY(VisibleAnywhere, Category = "Monster Card")
	URotatingMovementComponent* RotatingMovement;

	// GlowLight: 어두운 맵에서도 카드가 눈에 띄도록 은은하게 비추는 빛
	UPROPERTY(VisibleAnywhere, Category = "Monster Card")
	UPointLightComponent* GlowLight;

	// HoverHeight: 떨어진 위치 아래 바닥에서 얼마나 띄워 둘지(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	float HoverHeight = 60.f;
};
