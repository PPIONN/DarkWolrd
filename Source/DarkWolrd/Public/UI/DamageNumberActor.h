// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;
class UDamageNumberWidget;

// ADamageNumberActor: 피해 숫자 하나. 화면에 고정 크기로 그려지며(Screen 공간), 톡 튀어나온 뒤 위로 떠오르면서 사라짐
UCLASS()
class DARKWOLRD_API ADamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	ADamageNumberActor();

	// SpawnDamageNumber: Location에 피해 숫자를 띄움 (몬스터 TakeDamage 등에서 호출)
	static void SpawnDamageNumber(UWorld* World, const FVector& Location, float Damage, const FLinearColor& Color, int32 FontSize);

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Damage Number")
	UWidgetComponent* WidgetComponent;

	// LifeTime: 떠 있는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Damage Number")
	float LifeTime = 0.9f;

	// RiseDistance: 사라질 때까지 위로 떠오르는 높이(cm)
	UPROPERTY(EditAnywhere, Category = "Damage Number")
	float RiseDistance = 80.f;

	// PopScale / PopTime: 처음 나타날 때 이 배율로 크게 보였다가 PopTime초 동안 원래 크기로 줄어듦
	UPROPERTY(EditAnywhere, Category = "Damage Number")
	float PopScale = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Damage Number")
	float PopTime = 0.12f;

private:
	float Age = 0.f;
	FVector StartLocation = FVector::ZeroVector;

	// SideDrift: 숫자끼리 겹치지 않도록 옆으로 살짝 흘러가는 방향·거리
	FVector SideDrift = FVector::ZeroVector;
};
