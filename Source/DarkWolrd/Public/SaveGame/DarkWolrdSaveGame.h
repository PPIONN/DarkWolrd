// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DarkWolrdSaveGame.generated.h"

// UDarkWolrdSaveGame: 캐릭터 한 명의 세이브 데이터.
// CharacterStatComponent가 들고 있는 값들을 그대로 대응시켜서, 불러올 때 1:1로 복원할 수 있게 함
UCLASS()
class DARKWOLRD_API UDarkWolrdSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// SlotName: 이 세이브가 저장된 슬롯 이름 (SaveGameToSlot에 쓴 이름과 동일하게 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SlotName;

	// CharacterName: 캐릭터 선택창에 표시할 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentEXP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentMP = 15.f;

	// LevelAccumulatedMaxMP: 레벨업 시점의 지능에 따라 누적된 값이라 순수 계산식으로 복원이 안 되므로 그대로 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LevelAccumulatedMaxMP = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagicPower = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Intelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Vitality = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BaseDefense = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Money = 0;
};
