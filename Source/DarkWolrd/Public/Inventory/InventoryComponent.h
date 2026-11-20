// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ItemDefinition.h"
#include "InventoryComponent.generated.h"

class UCharacterStatComponent;

// FInventorySlot: 인벤토리 칸 하나 (아이템 종류 + 개수). Item이 없으면 빈 칸
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemDefinition* Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;

	bool IsEmpty() const { return Item == nullptr || Count <= 0; }
};

// FItemStack: 아이템 묶음 (시작 아이템 목록 등)
USTRUCT(BlueprintType)
struct FItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UItemDefinition* Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Count = 1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemAdded, UItemDefinition*, Item, int32, Count);

// UInventoryComponent: 플레이어가 가진 아이템(칸 Capacity개)과 장착한 장비(6칸)를 관리.
// 장비를 바꾸면 능력치 보너스를 CharacterStatComponent의 EquipmentBonus* 값에 다시 계산해 넣음(기본 능력치와 따로 저장되므로 중복으로 쌓이지 않음).
// 몬스터 카드는 여기 들어가지 않고 MonsterCompanionComponent가 그대로 관리함(인벤토리 화면의 '카드' 탭이 거기서 읽어 보여줌)
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// 장비 칸 번호: 0 무기, 1 머리, 2 몸, 3 신발, 4·5 장신구
	static constexpr int32 NumEquipSlots = 6;
	static EEquipSlot GetSlotTypeForEquipIndex(int32 EquipIndex);

	// Capacity: 인벤토리 칸 수 (화면은 7칸 × 6줄)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Capacity = 42;

	// StartingItems / StartingEquipment: 처음 시작할 때 받는 아이템과 끼고 시작하는 장비. 세이브나 레벨 이동 상태가 있으면 그걸로 덮어씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FItemStack> StartingItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<UItemDefinition*> StartingEquipment;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> Slots;

	// Equipped: 장비 칸 6개 (비어 있으면 nullptr)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<UItemDefinition*> Equipped;

	// OnInventoryChanged: 칸이나 장비가 바뀔 때마다 (화면 갱신용)
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	// OnItemAdded: 아이템을 새로 얻었을 때 (획득 알림용). 칸 이동·장비 해제처럼 원래 가진 걸 옮길 때는 안 불림
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemAdded OnItemAdded;

	// AddItem: 같은 아이템 칸에 먼저 쌓고 남으면 빈 칸에 넣음. 실제로 넣은 개수를 돌려줌(칸이 모자라면 Count보다 작음)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDefinition* Item, int32 Count = 1);

	// RemoveItem: 그 아이템을 Count개 뺌. 모자라면 아무것도 안 하고 false
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemDefinition* Item, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount(UItemDefinition* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetUsedSlotCount() const;

	// UseSlot: 소모품이면 사용(회복), 장비면 장착. 못 쓰면 false와 이유(OutMessage)
	bool UseSlot(int32 SlotIndex, FText& OutMessage);

	// EquipFromSlot: 인벤토리 칸의 장비를 장착. TargetEquipIndex가 -1이면 부위에 맞는 칸(장신구는 빈 칸 먼저)을 고름.
	// 원래 끼고 있던 장비는 그 인벤토리 칸으로 돌아옴
	bool EquipFromSlot(int32 SlotIndex, int32 TargetEquipIndex = -1);

	// Unequip: 장비를 빼서 인벤토리에 넣음. TargetSlot이 빈 칸이면 거기, 아니면 첫 빈 칸. 칸이 없으면 false
	bool Unequip(int32 EquipIndex, int32 TargetSlot = -1);

	// MoveSlot: 칸 옮기기. 같은 아이템이면 합치고, 아니면 맞바꿈
	void MoveSlot(int32 FromIndex, int32 ToIndex);

	// SwapEquipped: 장비 칸끼리 맞바꿈 (장신구 두 칸 사이에서만 의미 있음)
	bool SwapEquipped(int32 FromEquipIndex, int32 ToEquipIndex);

	// SortSlots: 같은 아이템을 합치고 장비 → 소모품 → 재료, 희귀도 높은 순으로 정렬
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SortSlots();

	// SetContents: 세이브·레벨 이동 상태를 그대로 적용 (지금 가진 것은 모두 바뀜)
	void SetContents(const TArray<FInventorySlot>& NewSlots, const TArray<UItemDefinition*>& NewEquipped);

	// GetEquipmentBonus: 장착한 장비 능력치 합계
	FItemStatBonus GetEquipmentBonus() const;

	// CanEquipAt: 그 장비 칸에 이 아이템을 낄 수 있는지
	static bool CanEquipAt(const UItemDefinition* Item, int32 EquipIndex);

protected:
	virtual void BeginPlay() override;

private:
	// ApplyEquipmentStats: 장비 능력치 합계를 스탯 컴포넌트의 EquipmentBonus* 값에 넣음
	void ApplyEquipmentStats();

	void NotifyChanged();

	UCharacterStatComponent* GetStatComponent() const;
};
