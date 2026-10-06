// Fill out your copyright notice in the Description page of Project Settings.

#include "DarkWolrdGameInstance.h"
#include "SaveGame/DarkWolrdSaveGame.h"
#include "SaveGame/CharacterManifestSaveGame.h"
#include "Stats/CharacterStatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

// 게임 인스턴스 시작: 특별한 초기화 없음 (자동저장은 캐릭터가 지정된 뒤 시작됨)
void UDarkWolrdGameInstance::Init()
{
	Super::Init();
}

// 게임 인스턴스 종료: 종료 직전에 마지막으로 한 번 더 저장해두고 타이머 정리
void UDarkWolrdGameInstance::Shutdown()
{
	if (!ActiveCharacterSlotName.IsEmpty())
	{
		SaveActiveCharacterNow();
	}

	StopAutoSaveTimer();

	Super::Shutdown();
}


// 매니페스트 슬롯 이름: 캐릭터 목록을 저장하는 고정된 슬롯 이름
FString UDarkWolrdGameInstance::GetManifestSlotName()
{
	return TEXT("CharacterManifest");
}


// 모든 캐릭터 슬롯 이름 조회: 매니페스트가 없으면 빈 배열
TArray<FString> UDarkWolrdGameInstance::GetAllCharacterSlotNames() const
{
	if (UCharacterManifestSaveGame* Manifest = Cast<UCharacterManifestSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GetManifestSlotName(), SaveUserIndex)))
	{
		return Manifest->CharacterSlotNames;
	}

	return TArray<FString>();
}


// 새 캐릭터 생성: 기본 스탯(UDarkWolrdSaveGame 기본값 = 레벨 1 기준)으로 세이브를 만들고,
// 즉시 저장한 뒤 매니페스트에도 슬롯 이름을 등록
UDarkWolrdSaveGame* UDarkWolrdGameInstance::CreateNewCharacter(const FString& CharacterName)
{
	UDarkWolrdSaveGame* NewSave = Cast<UDarkWolrdSaveGame>(UGameplayStatics::CreateSaveGameObject(UDarkWolrdSaveGame::StaticClass()));
	if (!NewSave) return nullptr;

	// 슬롯 이름은 캐릭터 이름을 그대로 사용 (동명 캐릭터 중복 처리는 추후 보완 예정)
	NewSave->SlotName = CharacterName;
	NewSave->CharacterName = CharacterName;

	UGameplayStatics::SaveGameToSlot(NewSave, NewSave->SlotName, SaveUserIndex);

	UCharacterManifestSaveGame* Manifest = Cast<UCharacterManifestSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GetManifestSlotName(), SaveUserIndex));

	if (!Manifest)
	{
		Manifest = Cast<UCharacterManifestSaveGame>(UGameplayStatics::CreateSaveGameObject(UCharacterManifestSaveGame::StaticClass()));
	}

	Manifest->CharacterSlotNames.AddUnique(NewSave->SlotName);
	UGameplayStatics::SaveGameToSlot(Manifest, GetManifestSlotName(), SaveUserIndex);

	return NewSave;
}


// 캐릭터 불러오기: 해당 슬롯이 없으면 nullptr 반환
UDarkWolrdSaveGame* UDarkWolrdGameInstance::LoadCharacter(const FString& SlotName) const
{
	return Cast<UDarkWolrdSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
}


// 세이브 데이터를 스탯 컴포넌트에 복원
void UDarkWolrdGameInstance::ApplySaveDataToStatComponent(const UDarkWolrdSaveGame* SaveData, UCharacterStatComponent* StatComponent) const
{
	if (!SaveData || !StatComponent) return;

	StatComponent->Level = SaveData->Level;
	StatComponent->CurrentEXP = SaveData->CurrentEXP;
	StatComponent->CurrentHP = SaveData->CurrentHP;
	StatComponent->CurrentMP = SaveData->CurrentMP;
	StatComponent->LevelAccumulatedMaxMP = SaveData->LevelAccumulatedMaxMP;
	StatComponent->MagicPower = SaveData->MagicPower;
	StatComponent->Intelligence = SaveData->Intelligence;
	StatComponent->Vitality = SaveData->Vitality;
	StatComponent->BaseDefense = SaveData->BaseDefense;
	StatComponent->Money = SaveData->Money;
}


// 활성 캐릭터 지정: 이후 자동저장 타이머가 이 슬롯을 대상으로 반복 저장함
void UDarkWolrdGameInstance::SetActiveCharacter(const FString& SlotName)
{
	ActiveCharacterSlotName = SlotName;
	StartAutoSaveTimer();
}


// 즉시 저장: 현재 플레이어가 조종 중인 캐릭터의 StatComponent 값을 읽어 슬롯에 덮어씀
void UDarkWolrdGameInstance::SaveActiveCharacterNow()
{
	if (ActiveCharacterSlotName.IsEmpty()) return;

	APlayerController* PC = GetFirstLocalPlayerController();
	ACharacter* PlayerCharacter = PC ? PC->GetCharacter() : nullptr;
	UCharacterStatComponent* StatComponent = PlayerCharacter ? PlayerCharacter->FindComponentByClass<UCharacterStatComponent>() : nullptr;

	if (!StatComponent) return;

	UDarkWolrdSaveGame* SaveData = Cast<UDarkWolrdSaveGame>(UGameplayStatics::CreateSaveGameObject(UDarkWolrdSaveGame::StaticClass()));
	if (!SaveData) return;

	SaveData->SlotName = ActiveCharacterSlotName;
	SaveData->CharacterName = ActiveCharacterSlotName;
	SaveData->Level = StatComponent->Level;
	SaveData->CurrentEXP = StatComponent->CurrentEXP;
	SaveData->CurrentHP = StatComponent->CurrentHP;
	SaveData->CurrentMP = StatComponent->CurrentMP;
	SaveData->LevelAccumulatedMaxMP = StatComponent->LevelAccumulatedMaxMP;
	SaveData->MagicPower = StatComponent->MagicPower;
	SaveData->Intelligence = StatComponent->Intelligence;
	SaveData->Vitality = StatComponent->Vitality;
	SaveData->BaseDefense = StatComponent->BaseDefense;
	SaveData->Money = StatComponent->Money;

	UGameplayStatics::SaveGameToSlot(SaveData, ActiveCharacterSlotName, SaveUserIndex);
}


// 자동저장 타이머 시작: 기존 타이머가 있으면 정리하고 새로 등록
void UDarkWolrdGameInstance::StartAutoSaveTimer()
{
	StopAutoSaveTimer();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AutoSaveTimerHandle, this, &UDarkWolrdGameInstance::SaveActiveCharacterNow, AutoSaveIntervalSeconds, true);
	}
}


// 자동저장 타이머 정지
void UDarkWolrdGameInstance::StopAutoSaveTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
	}
}
