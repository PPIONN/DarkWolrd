#include "Inventory/InventoryComponent.h"
#include "Stats/CharacterStatComponent.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// 시작: 칸을 만들고 시작 아이템·장비를 줌. 세이브나 레벨 이동 상태가 있으면 토끼 BeginPlay에서 SetContents로 덮어씀
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Slots.SetNum(Capacity);
	Equipped.SetNum(NumEquipSlots);

	for (const FItemStack& Stack : StartingItems)
	{
		if (Stack.Item)
		{
			const int32 Added = AddItem(Stack.Item, Stack.Count);
			(void)Added;
		}
	}
	for (UItemDefinition* Item : StartingEquipment)
	{
		if (!Item || !Item->IsEquipment()) continue;
		for (int32 EquipIndex = 0; EquipIndex < NumEquipSlots; ++EquipIndex)
		{
			if (!Equipped[EquipIndex] && CanEquipAt(Item, EquipIndex))
			{
				Equipped[EquipIndex] = Item;
				break;
			}
		}
	}
	ApplyEquipmentStats();

	// 시작 장비로 늘어난 최대 HP·MP만큼 채워서 가득 찬 채로 시작 (스탯 컴포넌트가 먼저 시작했으면 장비 전 최대치로 채워져 있음)
	if (UCharacterStatComponent* Stat = GetStatComponent())
	{
		if (Stat->HasBegunPlay())
		{
			Stat->CurrentHP = Stat->GetMaxHP();
			Stat->CurrentMP = Stat->GetMaxMP();
		}
	}
	NotifyChanged();
}


EEquipSlot UInventoryComponent::GetSlotTypeForEquipIndex(int32 EquipIndex)
{
	switch (EquipIndex)
	{
	case 0: return EEquipSlot::Weapon;
	case 1: return EEquipSlot::Head;
	case 2: return EEquipSlot::Body;
	case 3: return EEquipSlot::Feet;
	case 4:
	case 5: return EEquipSlot::Accessory;
	default: return EEquipSlot::None;
	}
}


bool UInventoryComponent::CanEquipAt(const UItemDefinition* Item, int32 EquipIndex)
{
	return Item && Item->IsEquipment() && Item->EquipSlot == GetSlotTypeForEquipIndex(EquipIndex);
}


int32 UInventoryComponent::AddItem(UItemDefinition* Item, int32 Count)
{
	if (!Item || Count <= 0) return 0;
	if (Slots.Num() != Capacity)
	{
		Slots.SetNum(Capacity);
	}

	const int32 MaxStack = FMath::Max(Item->MaxStack, 1);
	int32 Remaining = Count;

	// 같은 아이템이 있는 칸에 먼저 채움
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) break;
		if (Slot.Item == Item && Slot.Count < MaxStack)
		{
			const int32 Put = FMath::Min(MaxStack - Slot.Count, Remaining);
			Slot.Count += Put;
			Remaining -= Put;
		}
	}
	// 남으면 빈 칸에
	for (FInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0) break;
		if (Slot.IsEmpty())
		{
			const int32 Put = FMath::Min(MaxStack, Remaining);
			Slot.Item = Item;
			Slot.Count = Put;
			Remaining -= Put;
		}
	}

	const int32 Added = Count - Remaining;
	if (Added > 0)
	{
		OnItemAdded.Broadcast(Item, Added);
		NotifyChanged();
	}
	return Added;
}


bool UInventoryComponent::RemoveItem(UItemDefinition* Item, int32 Count)
{
	if (!Item || Count <= 0 || GetItemCount(Item) < Count) return false;

	int32 Remaining = Count;
	// 뒤쪽 칸부터 빼서 앞쪽에 정리해 둔 묶음이 남게 함
	for (int32 Index = Slots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FInventorySlot& Slot = Slots[Index];
		if (Slot.Item != Item) continue;
		const int32 Take = FMath::Min(Slot.Count, Remaining);
		Slot.Count -= Take;
		Remaining -= Take;
		if (Slot.Count <= 0)
		{
			Slot = FInventorySlot();
		}
	}
	NotifyChanged();
	return true;
}


int32 UInventoryComponent::GetItemCount(UItemDefinition* Item) const
{
	int32 Total = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Item == Item)
		{
			Total += Slot.Count;
		}
	}
	return Total;
}


int32 UInventoryComponent::GetUsedSlotCount() const
{
	int32 Used = 0;
	for (const FInventorySlot& Slot : Slots)
	{
		Used += Slot.IsEmpty() ? 0 : 1;
	}
	return Used;
}


bool UInventoryComponent::UseSlot(int32 SlotIndex, FText& OutMessage)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return false;
	UItemDefinition* Item = Slots[SlotIndex].Item;

	if (Item->IsEquipment())
	{
		const bool bEquipped = EquipFromSlot(SlotIndex);
		if (!bEquipped)
		{
			OutMessage = NSLOCTEXT("Inventory", "CannotEquip", "장착할 수 없습니다");
		}
		return bEquipped;
	}

	if (!Item->IsConsumable())
	{
		OutMessage = NSLOCTEXT("Inventory", "NotUsable", "사용할 수 없는 아이템입니다 (재료)");
		return false;
	}

	UCharacterStatComponent* Stat = GetStatComponent();
	if (!Stat) return false;

	// 회복할 게 없으면 아끼도록 쓰지 않음
	const bool bHPHelps = Item->HealHP > 0.f && Stat->CurrentHP < Stat->GetMaxHP() - KINDA_SMALL_NUMBER;
	const bool bMPHelps = Item->HealMP > 0.f && Stat->CurrentMP < Stat->GetMaxMP() - KINDA_SMALL_NUMBER;
	if (!bHPHelps && !bMPHelps)
	{
		OutMessage = Item->HealHP > 0.f && Item->HealMP <= 0.f ? NSLOCTEXT("Inventory", "HPFull", "HP가 이미 가득합니다")
			: Item->HealMP > 0.f && Item->HealHP <= 0.f ? NSLOCTEXT("Inventory", "MPFull", "MP가 이미 가득합니다")
			: NSLOCTEXT("Inventory", "BothFull", "HP와 MP가 이미 가득합니다");
		return false;
	}

	Stat->RestoreHP(Item->HealHP);
	Stat->RestoreMP(Item->HealMP);

	FInventorySlot& Slot = Slots[SlotIndex];
	if (--Slot.Count <= 0)
	{
		Slot = FInventorySlot();
	}
	OutMessage = FText::Format(NSLOCTEXT("Inventory", "Used", "{0}을(를) 사용했습니다"), Item->DisplayName);
	NotifyChanged();
	return true;
}


bool UInventoryComponent::EquipFromSlot(int32 SlotIndex, int32 TargetEquipIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return false;
	UItemDefinition* Item = Slots[SlotIndex].Item;
	if (!Item->IsEquipment()) return false;

	if (TargetEquipIndex < 0)
	{
		// 부위에 맞는 칸: 장신구는 빈 칸 먼저, 둘 다 차 있으면 첫 칸과 바꿈
		int32 FirstMatch = -1;
		for (int32 EquipIndex = 0; EquipIndex < NumEquipSlots; ++EquipIndex)
		{
			if (!CanEquipAt(Item, EquipIndex)) continue;
			if (FirstMatch < 0)
			{
				FirstMatch = EquipIndex;
			}
			if (!Equipped[EquipIndex])
			{
				TargetEquipIndex = EquipIndex;
				break;
			}
		}
		if (TargetEquipIndex < 0)
		{
			TargetEquipIndex = FirstMatch;
		}
	}
	if (!CanEquipAt(Item, TargetEquipIndex)) return false;

	UItemDefinition* Previous = Equipped[TargetEquipIndex];
	Equipped[TargetEquipIndex] = Item;

	// 장비는 한 칸에 1개. 혹시 여러 개 쌓여 있으면 하나만 빼고, 빼던 장비는 빈 칸으로
	FInventorySlot& Slot = Slots[SlotIndex];
	if (Slot.Count > 1)
	{
		--Slot.Count;
		if (Previous && AddItem(Previous, 1) == 0)
		{
			// 넣을 칸이 없으면 되돌림
			Equipped[TargetEquipIndex] = Previous;
			++Slot.Count;
			return false;
		}
	}
	else
	{
		Slot.Item = Previous;
		Slot.Count = Previous ? 1 : 0;
	}

	ApplyEquipmentStats();
	NotifyChanged();
	return true;
}


bool UInventoryComponent::Unequip(int32 EquipIndex, int32 TargetSlot)
{
	if (!Equipped.IsValidIndex(EquipIndex) || !Equipped[EquipIndex]) return false;
	UItemDefinition* Item = Equipped[EquipIndex];

	// 놓은 칸에 같은 부위 장비가 있으면 맞바꿈
	if (Slots.IsValidIndex(TargetSlot) && !Slots[TargetSlot].IsEmpty())
	{
		if (CanEquipAt(Slots[TargetSlot].Item, EquipIndex) && Slots[TargetSlot].Count == 1)
		{
			Equipped[EquipIndex] = Slots[TargetSlot].Item;
			Slots[TargetSlot].Item = Item;
			ApplyEquipmentStats();
			NotifyChanged();
			return true;
		}
		TargetSlot = -1;
	}
	if (!Slots.IsValidIndex(TargetSlot))
	{
		TargetSlot = Slots.IndexOfByPredicate([](const FInventorySlot& Slot) { return Slot.IsEmpty(); });
	}
	if (!Slots.IsValidIndex(TargetSlot)) return false;

	Slots[TargetSlot].Item = Item;
	Slots[TargetSlot].Count = 1;
	Equipped[EquipIndex] = nullptr;
	ApplyEquipmentStats();
	NotifyChanged();
	return true;
}


void UInventoryComponent::MoveSlot(int32 FromIndex, int32 ToIndex)
{
	if (FromIndex == ToIndex || !Slots.IsValidIndex(FromIndex) || !Slots.IsValidIndex(ToIndex) || Slots[FromIndex].IsEmpty()) return;

	FInventorySlot& From = Slots[FromIndex];
	FInventorySlot& To = Slots[ToIndex];
	if (To.Item == From.Item)
	{
		const int32 MaxStack = FMath::Max(From.Item->MaxStack, 1);
		const int32 Put = FMath::Min(MaxStack - To.Count, From.Count);
		To.Count += Put;
		From.Count -= Put;
		if (From.Count <= 0)
		{
			From = FInventorySlot();
		}
	}
	else
	{
		Swap(From, To);
	}
	NotifyChanged();
}


bool UInventoryComponent::SwapEquipped(int32 FromEquipIndex, int32 ToEquipIndex)
{
	if (!Equipped.IsValidIndex(FromEquipIndex) || !Equipped.IsValidIndex(ToEquipIndex) || FromEquipIndex == ToEquipIndex) return false;
	if (Equipped[FromEquipIndex] && !CanEquipAt(Equipped[FromEquipIndex], ToEquipIndex)) return false;
	if (Equipped[ToEquipIndex] && !CanEquipAt(Equipped[ToEquipIndex], FromEquipIndex)) return false;

	Swap(Equipped[FromEquipIndex], Equipped[ToEquipIndex]);
	NotifyChanged();
	return true;
}


void UInventoryComponent::SortSlots()
{
	// 같은 아이템 합치기
	TArray<FInventorySlot> Merged;
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty()) continue;
		const int32 MaxStack = FMath::Max(Slot.Item->MaxStack, 1);
		int32 Remaining = Slot.Count;
		for (FInventorySlot& Existing : Merged)
		{
			if (Remaining <= 0) break;
			if (Existing.Item == Slot.Item && Existing.Count < MaxStack)
			{
				const int32 Put = FMath::Min(MaxStack - Existing.Count, Remaining);
				Existing.Count += Put;
				Remaining -= Put;
			}
		}
		while (Remaining > 0)
		{
			const int32 Put = FMath::Min(MaxStack, Remaining);
			Merged.Add({ Slot.Item, Put });
			Remaining -= Put;
		}
	}

	Merged.StableSort([](const FInventorySlot& A, const FInventorySlot& B)
	{
		if (A.Item->Category != B.Item->Category) return A.Item->Category < B.Item->Category;
		if (A.Item->Rarity != B.Item->Rarity) return A.Item->Rarity > B.Item->Rarity;
		if (A.Item->SortOrder != B.Item->SortOrder) return A.Item->SortOrder < B.Item->SortOrder;
		if (A.Item != B.Item) return A.Item->DisplayName.CompareTo(B.Item->DisplayName) < 0;
		return A.Count > B.Count;
	});

	Merged.SetNum(Capacity);
	Slots = MoveTemp(Merged);
	NotifyChanged();
}


void UInventoryComponent::SetContents(const TArray<FInventorySlot>& NewSlots, const TArray<UItemDefinition*>& NewEquipped)
{
	Slots = NewSlots;
	Slots.SetNum(Capacity);
	for (FInventorySlot& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Slot = FInventorySlot();
		}
	}

	Equipped.Init(nullptr, NumEquipSlots);
	for (int32 EquipIndex = 0; EquipIndex < NumEquipSlots && EquipIndex < NewEquipped.Num(); ++EquipIndex)
	{
		if (CanEquipAt(NewEquipped[EquipIndex], EquipIndex))
		{
			Equipped[EquipIndex] = NewEquipped[EquipIndex];
		}
	}
	ApplyEquipmentStats();
	NotifyChanged();
}


FItemStatBonus UInventoryComponent::GetEquipmentBonus() const
{
	FItemStatBonus Total;
	for (const UItemDefinition* Item : Equipped)
	{
		if (Item && Item->IsEquipment())
		{
			Total += Item->StatBonus;
		}
	}
	return Total;
}


void UInventoryComponent::ApplyEquipmentStats()
{
	if (UCharacterStatComponent* Stat = GetStatComponent())
	{
		Stat->SetEquipmentBonus(GetEquipmentBonus());
	}
}


void UInventoryComponent::NotifyChanged()
{
	OnInventoryChanged.Broadcast();
}


UCharacterStatComponent* UInventoryComponent::GetStatComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UCharacterStatComponent>() : nullptr;
}
