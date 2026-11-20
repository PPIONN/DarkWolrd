// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ItemDefinition.h"

FItemStatBonus& FItemStatBonus::operator+=(const FItemStatBonus& Other)
{
	MagicPower += Other.MagicPower;
	Intelligence += Other.Intelligence;
	Vitality += Other.Vitality;
	Defense += Other.Defense;
	MaxHP += Other.MaxHP;
	MaxMP += Other.MaxMP;
	AttackSpeed += Other.AttackSpeed;
	MoveSpeed += Other.MoveSpeed;
	return *this;
}


void FItemStatBonus::AppendLines(TArray<FText>& OutLines) const
{
	const auto AddInt = [&OutLines](const TCHAR* Label, int32 Value)
	{
		if (Value != 0)
		{
			OutLines.Add(FText::FromString(FString::Printf(TEXT("%s %+d"), Label, Value)));
		}
	};
	const auto AddPercent = [&OutLines](const TCHAR* Label, float Value)
	{
		if (!FMath::IsNearlyZero(Value))
		{
			OutLines.Add(FText::FromString(FString::Printf(TEXT("%s %+d%%"), Label, FMath::RoundToInt(Value * 100.f))));
		}
	};

	AddInt(TEXT("마력"), MagicPower);
	AddInt(TEXT("지능"), Intelligence);
	AddInt(TEXT("체력"), Vitality);
	AddInt(TEXT("방어력"), Defense);
	AddInt(TEXT("최대 HP"), FMath::RoundToInt(MaxHP));
	AddInt(TEXT("최대 MP"), FMath::RoundToInt(MaxMP));
	AddPercent(TEXT("공격속도"), AttackSpeed);
	AddPercent(TEXT("이동속도"), MoveSpeed);
}


TArray<FText> UItemDefinition::GetEffectLines() const
{
	TArray<FText> Lines;
	if (IsConsumable())
	{
		if (HealHP > 0.f)
		{
			Lines.Add(FText::FromString(FString::Printf(TEXT("HP %d 회복"), FMath::RoundToInt(HealHP))));
		}
		if (HealMP > 0.f)
		{
			Lines.Add(FText::FromString(FString::Printf(TEXT("MP %d 회복"), FMath::RoundToInt(HealMP))));
		}
	}
	else if (IsEquipment())
	{
		StatBonus.AppendLines(Lines);
	}
	return Lines;
}


FLinearColor UItemDefinition::GetRarityColor(EItemRarity InRarity)
{
	switch (InRarity)
	{
	case EItemRarity::Uncommon:		return FLinearColor(0.35f, 0.85f, 0.45f);
	case EItemRarity::Rare:			return FLinearColor(0.3f, 0.6f, 1.f);
	case EItemRarity::Epic:			return FLinearColor(0.72f, 0.42f, 1.f);
	case EItemRarity::Legendary:	return FLinearColor(1.f, 0.7f, 0.2f);
	default:						return FLinearColor(0.78f, 0.8f, 0.86f);
	}
}


// 이름은 직접 적음: UENUM의 DisplayName 메타데이터는 에디터 전용이라 패키징한 게임에서는 영어 이름이 나옴
FText UItemDefinition::GetRarityName(EItemRarity InRarity)
{
	switch (InRarity)
	{
	case EItemRarity::Uncommon:		return NSLOCTEXT("Item", "RarityUncommon", "고급");
	case EItemRarity::Rare:			return NSLOCTEXT("Item", "RarityRare", "희귀");
	case EItemRarity::Epic:			return NSLOCTEXT("Item", "RarityEpic", "영웅");
	case EItemRarity::Legendary:	return NSLOCTEXT("Item", "RarityLegendary", "전설");
	default:						return NSLOCTEXT("Item", "RarityCommon", "일반");
	}
}


FText UItemDefinition::GetCategoryName(EItemCategory InCategory)
{
	switch (InCategory)
	{
	case EItemCategory::Equipment:	return NSLOCTEXT("Item", "CategoryEquipment", "장비");
	case EItemCategory::Consumable:	return NSLOCTEXT("Item", "CategoryConsumable", "소모품");
	default:						return NSLOCTEXT("Item", "CategoryMaterial", "재료");
	}
}


FText UItemDefinition::GetEquipSlotName(EEquipSlot InSlot)
{
	switch (InSlot)
	{
	case EEquipSlot::Weapon:	return NSLOCTEXT("Item", "SlotWeapon", "무기");
	case EEquipSlot::Head:		return NSLOCTEXT("Item", "SlotHead", "머리");
	case EEquipSlot::Body:		return NSLOCTEXT("Item", "SlotBody", "몸");
	case EEquipSlot::Feet:		return NSLOCTEXT("Item", "SlotFeet", "신발");
	case EEquipSlot::Accessory:	return NSLOCTEXT("Item", "SlotAccessory", "장신구");
	default:					return FText::GetEmpty();
	}
}
