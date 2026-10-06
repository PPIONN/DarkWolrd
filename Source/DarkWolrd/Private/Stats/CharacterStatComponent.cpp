// Fill out your copyright notice in the Description page of Project Settings.

#include "Stats/CharacterStatComponent.h"
#include "TimerManager.h"

UCharacterStatComponent::UCharacterStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 게임 시작 시: 현재 HP/MP를 최대치로 채우고, RegenInterval마다 반복되는 재생 타이머를 시작
void UCharacterStatComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = GetMaxHP();
	CurrentMP = GetMaxMP();

	GetWorld()->GetTimerManager().SetTimer(RegenTimerHandle, this, &UCharacterStatComponent::ApplyRegenTick, RegenInterval, true);
}


// RegenInterval마다 호출: HPRegenAmount/MPRegenAmount(+소환수 패시브)만큼 회복시키되 최대치를 넘기지 않음
void UCharacterStatComponent::ApplyRegenTick()
{
	CurrentHP = FMath::Clamp(CurrentHP + HPRegenAmount + SummonBonusHPRegen, 0.f, GetMaxHP());
	CurrentMP = FMath::Clamp(CurrentMP + MPRegenAmount + SummonBonusMPRegen, 0.f, GetMaxMP());
}


// 다음 레벨까지 필요한 경험치 (임시 곡선: 레벨 * 100)
int32 UCharacterStatComponent::GetEXPToNextLevel() const
{
	return Level * 100;
}


// 경험치 추가: 필요한 만큼 LevelUp을 반복 호출
void UCharacterStatComponent::AddExperience(int32 Amount)
{
	if (Amount <= 0 || Level >= MaxLevel) return;

	CurrentEXP += Amount;

	while (Level < MaxLevel && CurrentEXP >= GetEXPToNextLevel())
	{
		CurrentEXP -= GetEXPToNextLevel();
		LevelUp();
	}
}


// 레벨업: 레벨 증가, MP는 그 시점의 지능을 반영해 누적, HP/MP는 최대치로 회복
void UCharacterStatComponent::LevelUp()
{
	if (Level >= MaxLevel) return;

	++Level;
	LevelAccumulatedMaxMP += MPPerLevel + FMath::FloorToFloat(Intelligence / IntelligencePerBonusMP);

	CurrentHP = GetMaxHP();
	CurrentMP = GetMaxMP();

	OnLevelUp.Broadcast();
}


// 최대 HP: 기본값 + 레벨 보너스 + 체력 보너스 + 장비 보너스 + 소환수 패시브
float UCharacterStatComponent::GetMaxHP() const
{
	return BaseMaxHP + (Level - 1) * HPPerLevel + Vitality * HPPerVitality + EquipmentBonusHP + SummonBonusMaxHP;
}


// 최대 MP: 레벨업마다 누적된 값 + 장비 보너스 + 소환수 패시브
float UCharacterStatComponent::GetMaxMP() const
{
	return LevelAccumulatedMaxMP + EquipmentBonusMP + SummonBonusMaxMP;
}


// MP 회복: 지정한 양만큼 회복시키되 최대치를 넘지 않음
void UCharacterStatComponent::RestoreMP(float Amount)
{
	if (Amount <= 0.f) return;

	CurrentMP = FMath::Clamp(CurrentMP + Amount, 0.f, GetMaxMP());
}


// MP 소비: 충분하면 차감 후 true, 부족하면 아무 것도 하지 않고 false (스킬 코스트 등에서 사용)
bool UCharacterStatComponent::SpendMP(float Amount)
{
	if (Amount <= 0.f || CurrentMP < Amount) return false;

	CurrentMP -= Amount;
	return true;
}


// 총 방어력: 순수 방어력 + 체력에서 파생된 방어력(체력 2당 1) + 소환수 패시브
float UCharacterStatComponent::GetTotalDefense() const
{
	return BaseDefense + FMath::FloorToFloat(Vitality / 2.f) + SummonBonusDefense;
}


// 최종 피해량 계산: ((방어자 레벨×10) - (공격자 레벨×10) + (공격자 마력 + 공격자 지능/2) - 방어자 방어력) × DamageMultiplier.
// 그 값이 0 이하면 최소 피해, 임계값 이하로 작으면 잉여 대미지 보정
float UCharacterStatComponent::CalculateIncomingDamage(int32 AttackerLevel, float AttackerMagicPower, float AttackerIntelligence, float DamageMultiplier) const
{
	const float LevelDiffBonus = (Level - AttackerLevel) * 10.f;
	const float RawDamage = (LevelDiffBonus + AttackerMagicPower + (AttackerIntelligence / 2.f) - GetTotalDefense()) * DamageMultiplier;

	float FinalDamage;
	if (RawDamage <= 0.f)
	{
		FinalDamage = 1.f;
	}
	else if (RawDamage <= NearZeroDamageThreshold)
	{
		FinalDamage = RawDamage + FMath::FRandRange(SurplusDamageMin, SurplusDamageMax);
	}
	else
	{
		FinalDamage = RawDamage;
	}

	return FinalDamage * FMath::FRandRange(1.f, 1.f + DamageRandomVarianceMax);
}


// 대미지 적용: 계산된 값을 실제로 CurrentHP에서 차감. 0 미만으로는 내려가지 않음
void UCharacterStatComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f) return;

	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, GetMaxHP());
}


// 최종 공격속도 배율: 기본값 + 장비 보너스 + 패시브 보너스 + 소환수 패시브. 0.1 미만으로는 내려가지 않게 클램프
float UCharacterStatComponent::GetAttackSpeedMultiplier() const
{
	return FMath::Max(BaseAttackSpeedMultiplier + EquipmentBonusAttackSpeed + PassiveBonusAttackSpeed + SummonBonusAttackSpeed, 0.1f);
}


void UCharacterStatComponent::ClearSummonPassives()
{
	SummonBonusMaxHP = 0.f;
	SummonBonusMaxMP = 0.f;
	SummonBonusDefense = 0.f;
	SummonBonusHPRegen = 0.f;
	SummonBonusMPRegen = 0.f;
	SummonBonusAttackSpeed = 0.f;
	SummonBonusMoveSpeed = 0.f;
}


void UCharacterStatComponent::AddSummonPassive(ESummonPassiveType Type, float Value)
{
	switch (Type)
	{
	case ESummonPassiveType::MaxHP:			SummonBonusMaxHP += Value; break;
	case ESummonPassiveType::MaxMP:			SummonBonusMaxMP += Value; break;
	case ESummonPassiveType::Defense:		SummonBonusDefense += Value; break;
	case ESummonPassiveType::HPRegen:		SummonBonusHPRegen += Value; break;
	case ESummonPassiveType::MPRegen:		SummonBonusMPRegen += Value; break;
	case ESummonPassiveType::AttackSpeed:	SummonBonusAttackSpeed += Value; break;
	case ESummonPassiveType::MoveSpeed:		SummonBonusMoveSpeed += Value; break;
	default: break;
	}
}


// 이동속도 배율: 1 + 소환수 패시브. 0.1 미만으로는 내려가지 않게 클램프
float UCharacterStatComponent::GetMoveSpeedMultiplier() const
{
	return FMath::Max(1.f + SummonBonusMoveSpeed, 0.1f);
}


void UCharacterStatComponent::AddMoney(int64 Amount)
{
	if (Amount > 0)
	{
		Money += Amount;
	}
}


bool UCharacterStatComponent::SpendMoney(int64 Amount)
{
	if (Amount <= 0 || Money < Amount) return false;

	Money -= Amount;
	return true;
}
