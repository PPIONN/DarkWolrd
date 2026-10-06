// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CharacterManifestSaveGame.generated.h"

// UCharacterManifestSaveGame: 지금까지 생성된 모든 캐릭터 슬롯 이름을 모아두는 목록.
// UGameplayStatics에는 "모든 슬롯 나열" 기능이 없어서, 별도의 고정 슬롯("CharacterManifest")에
// 이 목록을 따로 저장해두고 캐릭터 선택창을 채울 때 사용한다
UCLASS()
class DARKWOLRD_API UCharacterManifestSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> CharacterSlotNames;
};
