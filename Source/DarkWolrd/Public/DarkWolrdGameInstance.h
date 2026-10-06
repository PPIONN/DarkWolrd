// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DarkWolrdGameInstance.generated.h"

class UDarkWolrdSaveGame;
class UCharacterStatComponent;
class UMonsterCompanionComponent;

// UDarkWolrdGameInstance: 캐릭터 세이브/자동저장을 관리하는 게임 인스턴스.
// 레벨 전환과 무관하게 게임 실행 내내 살아있어서, 자동저장 타이머를 여기서 돌린다.
UCLASS()
class DARKWOLRD_API UDarkWolrdGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// AutoSaveIntervalSeconds: 자동저장 주기(초). 기본 60초(1분)마다 저장
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float AutoSaveIntervalSeconds = 60.f;

	// SaveUserIndex: 세이브 슬롯 구분용 유저 인덱스. 로컬 싱글플레이어라 항상 0
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SaveUserIndex = 0;

	// ActiveCharacterSlotName: 현재 플레이 중인 캐릭터의 세이브 슬롯 이름 (없으면 빈 문자열)
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString ActiveCharacterSlotName;

	// GetAllCharacterSlotNames: 지금까지 생성된 모든 캐릭터 슬롯 이름 목록 (캐릭터 선택창에 사용)
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FString> GetAllCharacterSlotNames() const;

	// CreateNewCharacter: 새 캐릭터 세이브 슬롯을 기본 스탯으로 생성하고 즉시 저장, 목록에도 등록
	UFUNCTION(BlueprintCallable, Category = "Save")
	UDarkWolrdSaveGame* CreateNewCharacter(const FString& CharacterName);

	// LoadCharacter: 지정한 슬롯의 세이브 데이터를 불러옴 (없으면 nullptr)
	UFUNCTION(BlueprintCallable, Category = "Save")
	UDarkWolrdSaveGame* LoadCharacter(const FString& SlotName) const;

	// ApplySaveDataToStatComponent: 불러온 세이브 데이터를 스탯 컴포넌트에 그대로 복원
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySaveDataToStatComponent(const UDarkWolrdSaveGame* SaveData, UCharacterStatComponent* StatComponent) const;

	// ApplySaveDataToCompanionComponent: 몬스터 카드 보유 목록·도감·소환 슬롯을 복원. 클래스를 못 찾는 항목(삭제/이동된 몬스터 BP)은 건너뜀
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySaveDataToCompanionComponent(const UDarkWolrdSaveGame* SaveData, UMonsterCompanionComponent* CompanionComponent) const;

	// ApplySaveDataToPlayer: 플레이어 액터에 붙은 스탯·몬스터 카드 컴포넌트를 찾아 한 번에 복원 (위 두 함수를 함께 호출)
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySaveDataToPlayer(const UDarkWolrdSaveGame* SaveData, AActor* PlayerActor) const;

	// SetActiveCharacter: 이 슬롯을 현재 플레이 중인 캐릭터로 지정하고 자동저장 타이머를 시작
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SetActiveCharacter(const FString& SlotName);

	// SaveActiveCharacterNow: 현재 플레이 중인 캐릭터를 즉시 저장 (자동저장 타이머와 수동 저장 모두 여기로 옴)
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveActiveCharacterNow();

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	FTimerHandle AutoSaveTimerHandle;

	void StartAutoSaveTimer();
	void StopAutoSaveTimer();

	static FString GetManifestSlotName();
};
