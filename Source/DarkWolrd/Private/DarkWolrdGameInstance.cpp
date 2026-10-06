// Fill out your copyright notice in the Description page of Project Settings.

#include "DarkWolrdGameInstance.h"
#include "SaveGame/DarkWolrdSaveGame.h"
#include "SaveGame/CharacterManifestSaveGame.h"
#include "Stats/CharacterStatComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Monster/BaseMonster.h"
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


// 몬스터 카드 데이터 복원: 클래스가 비어 있는 항목(몬스터 BP가 삭제/이동돼 불러오지 못한 것)은 버림
void UDarkWolrdGameInstance::ApplySaveDataToCompanionComponent(const UDarkWolrdSaveGame* SaveData, UMonsterCompanionComponent* CompanionComponent) const
{
	if (!SaveData || !CompanionComponent) return;

	CompanionComponent->DespawnAllSummons();

	CompanionComponent->HeldCards = SaveData->HeldCards.FilterByPredicate(
		[](const FMonsterCardEntry& Card) { return Card.MonsterClass != nullptr; });

	CompanionComponent->RegisteredMonsters = SaveData->RegisteredMonsters.FilterByPredicate(
		[](const FRegisteredMonsterEntry& Entry) { return Entry.MonsterClass != nullptr; });

	CompanionComponent->SelectedSummonSlots = SaveData->SelectedSummonSlots.FilterByPredicate(
		[](const TSubclassOf<ABaseMonster>& MonsterClass) { return MonsterClass != nullptr; });
}


// 플레이어 전체 복원: 스탯과 몬스터 카드를 한 번에
void UDarkWolrdGameInstance::ApplySaveDataToPlayer(const UDarkWolrdSaveGame* SaveData, AActor* PlayerActor) const
{
	if (!SaveData || !PlayerActor) return;

	ApplySaveDataToStatComponent(SaveData, PlayerActor->FindComponentByClass<UCharacterStatComponent>());
	ApplySaveDataToCompanionComponent(SaveData, PlayerActor->FindComponentByClass<UMonsterCompanionComponent>());
}


// 활성 캐릭터 지정: 이후 자동저장 타이머가 이 슬롯을 대상으로 반복 저장함
void UDarkWolrdGameInstance::SetActiveCharacter(const FString& SlotName)
{
	ActiveCharacterSlotName = SlotName;
	StartAutoSaveTimer();
}


// 즉시 저장: 현재 플레이어가 조종 중인 캐릭터의 상태를 읽어 슬롯에 덮어씀
void UDarkWolrdGameInstance::SaveActiveCharacterNow()
{
	if (ActiveCharacterSlotName.IsEmpty()) return;

	APlayerController* PC = GetFirstLocalPlayerController();
	if (UDarkWolrdSaveGame* SaveData = BuildSaveDataFromCharacter(PC ? PC->GetCharacter() : nullptr, ActiveCharacterSlotName))
	{
		UGameplayStatics::SaveGameToSlot(SaveData, ActiveCharacterSlotName, SaveUserIndex);
	}
}


// 세이브 데이터 만들기: 스탯 컴포넌트가 없으면(플레이어가 아니면) nullptr
UDarkWolrdSaveGame* UDarkWolrdGameInstance::BuildSaveDataFromCharacter(ACharacter* PlayerCharacter, const FString& SlotName) const
{
	UCharacterStatComponent* StatComponent = PlayerCharacter ? PlayerCharacter->FindComponentByClass<UCharacterStatComponent>() : nullptr;
	if (!StatComponent) return nullptr;

	UDarkWolrdSaveGame* SaveData = Cast<UDarkWolrdSaveGame>(UGameplayStatics::CreateSaveGameObject(UDarkWolrdSaveGame::StaticClass()));
	if (!SaveData) return nullptr;

	SaveData->SlotName = SlotName;
	SaveData->CharacterName = SlotName;
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

	if (const UMonsterCompanionComponent* CompanionComponent = PlayerCharacter->FindComponentByClass<UMonsterCompanionComponent>())
	{
		SaveData->HeldCards = CompanionComponent->HeldCards;
		SaveData->RegisteredMonsters = CompanionComponent->RegisteredMonsters;
		SaveData->SelectedSummonSlots = CompanionComponent->SelectedSummonSlots;
	}

	return SaveData;
}


FString UDarkWolrdGameInstance::GetCurrentLevelPath() const
{
	const UWorld* World = GetWorld();
	return World ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
}


// 떠나기 전: 플레이어 상태를 메모리에 담고, 필요하면 돌아올 위치를 기억. 세이브 슬롯이 있으면 디스크에도 저장해둠
void UDarkWolrdGameInstance::PrepareLevelTravel(ACharacter* PlayerCharacter, bool bSetReturnPoint, const FTransform& InReturnTransform)
{
	PendingTravelSnapshot = BuildSaveDataFromCharacter(PlayerCharacter, ActiveCharacterSlotName);

	if (bSetReturnPoint)
	{
		ReturnLevelPath = GetCurrentLevelPath();
		ReturnTransform = InReturnTransform;
	}

	SaveActiveCharacterNow();
}


// 돌아가기: 기억해둔 레벨을 열고, 도착하면 기억해둔 위치에 세우도록 표시
bool UDarkWolrdGameInstance::TravelToReturnPoint()
{
	if (ReturnLevelPath.IsEmpty()) return false;

	bPlaceAtReturnPointOnArrival = true;
	UGameplayStatics::OpenLevel(this, FName(*ReturnLevelPath));
	return true;
}


// 도착 후: 담아둔 상태를 적용하고, 돌아온 레벨이면 기억해둔 위치(입구 포털 앞)에 세움
void UDarkWolrdGameInstance::RestorePlayerAfterTravel(ACharacter* PlayerCharacter)
{
	if (!PlayerCharacter) return;

	if (PendingTravelSnapshot)
	{
		ApplySaveDataToPlayer(PendingTravelSnapshot, PlayerCharacter);
		PendingTravelSnapshot = nullptr;
	}

	if (bPlaceAtReturnPointOnArrival && GetCurrentLevelPath() == ReturnLevelPath)
	{
		PlayerCharacter->SetActorTransform(ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* Controller = PlayerCharacter->GetController())
		{
			Controller->SetControlRotation(ReturnTransform.Rotator());
		}
	}
	bPlaceAtReturnPointOnArrival = false;
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
