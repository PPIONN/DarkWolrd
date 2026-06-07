// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HP_Potion.generated.h"

UCLASS()
class DARKWOLRD_API AHP_Potion : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHP_Potion();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	FString PotionName = TEXT("하급 포션"); // 포션 이름

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	FString PotionTooltip = TEXT("체력을 50 회복시켜줍니다.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	float HealHP = 50.0f; // 포션 성능
};
