// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UTexture2D;

// EItemCategory: 인벤토리 탭과 정렬 순서에 쓰는 아이템 분류
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Equipment	UMETA(DisplayName = "장비"),
	Consumable	UMETA(DisplayName = "소모품"),
	Material	UMETA(DisplayName = "재료"),
};

// EItemRarity: 희귀도. 칸 테두리 색과 이름 색으로 표시
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common		UMETA(DisplayName = "일반"),
	Uncommon	UMETA(DisplayName = "고급"),
	Rare		UMETA(DisplayName = "희귀"),
	Epic		UMETA(DisplayName = "영웅"),
	Legendary	UMETA(DisplayName = "전설"),
};

// EEquipSlot: 장비 부위. 장신구는 칸이 두 개
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
	None		UMETA(DisplayName = "없음"),
	Weapon		UMETA(DisplayName = "무기"),
	Head		UMETA(DisplayName = "머리"),
	Body		UMETA(DisplayName = "몸"),
	Feet		UMETA(DisplayName = "신발"),
	Accessory	UMETA(DisplayName = "장신구"),
};

// FItemStatBonus: 장비를 끼면 오르는 능력치. 저장되는 기본 능력치와 따로 더해짐(CharacterStatComponent의 EquipmentBonus*)
USTRUCT(BlueprintType)
struct FItemStatBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MagicPower = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Intelligence = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Vitality = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Defense = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxHP = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxMP = 0.f;

	// AttackSpeed: 공격속도 배율에 더함 (0.1 = 10% 빨라짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float AttackSpeed = 0.f;

	// MoveSpeed: 이동속도 배율에 더함 (0.05 = 5% 빨라짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MoveSpeed = 0.f;

	FItemStatBonus& operator+=(const FItemStatBonus& Other);

	// AppendLines: "마력 +3" 같은 줄을 OutLines에 추가 (0인 항목은 생략)
	void AppendLines(TArray<FText>& OutLines) const;
};

// UItemDefinition: 아이템 한 종류의 정의(이름·아이콘·분류·효과). 에셋 하나가 아이템 한 종류.
// 인벤토리 칸은 이 에셋을 가리키고 개수만 따로 가짐. 세이브에는 에셋 경로로 저장됨(에셋을 옮기면 그 아이템은 불러올 때 빠짐)
UCLASS(BlueprintType)
class DARKWOLRD_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	// Icon: 인벤토리 칸에 그릴 그림. 비워두면 칸에 이름 글자를 대신 씀
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	// MaxStack: 한 칸에 쌓을 수 있는 최대 개수 (장비는 1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 99;

	// SortOrder: 같은 분류·희귀도 안에서 정렬할 때 작은 값이 앞
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 SortOrder = 0;

	// ─── 소모품 ───
	// HealHP / HealMP: 사용하면 회복하는 양
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "Category == EItemCategory::Consumable"))
	float HealHP = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "Category == EItemCategory::Consumable"))
	float HealMP = 0.f;

	// ─── 장비 ───
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment", meta = (EditCondition = "Category == EItemCategory::Equipment"))
	EEquipSlot EquipSlot = EEquipSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment", meta = (EditCondition = "Category == EItemCategory::Equipment"))
	FItemStatBonus StatBonus;

	bool IsEquipment() const { return Category == EItemCategory::Equipment && EquipSlot != EEquipSlot::None; }
	bool IsConsumable() const { return Category == EItemCategory::Consumable; }

	// GetEffectLines: 상세 설명에 보여줄 효과 줄 (회복량, 장비 능력치)
	TArray<FText> GetEffectLines() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("Item"), GetFName()); }

	static FLinearColor GetRarityColor(EItemRarity InRarity);
	static FText GetRarityName(EItemRarity InRarity);
	static FText GetCategoryName(EItemCategory InCategory);
	static FText GetEquipSlotName(EEquipSlot InSlot);
};

// FItemDropEntry: 몬스터가 쓰러질 때 떨어뜨리는 아이템 한 줄 (확률과 개수 범위)
USTRUCT(BlueprintType)
struct FItemDropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemDefinition* Item = nullptr;

	// Chance: 떨어뜨릴 확률 (0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};
