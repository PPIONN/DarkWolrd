// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Summon/SummonTypes.h"
#include "CharacterStatComponent.generated.h"

// FOnCharacterLevelUp: 레벨업이 일어날 때마다 브로드캐스트. AbilityComponent가 이걸 구독해서
// 스킬 포인트를 지급하는 등, 서로 몰라도 되는 컴포넌트끼리 느슨하게 연결하는 용도
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterLevelUp);

// UCharacterStatComponent: 플레이어 캐릭터(토끼)의 레벨/스탯을 관리하는 컴포넌트.
// HP·MP는 레벨업 공식으로 자동 계산되고, 마력·지능·체력·방어력은 장비/퀘스트 보상/몬스터 처치 등
// 외부 이벤트로만 증가한다(레벨업으로는 증가하지 않음).
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UCharacterStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterStatComponent();

	// ─── 레벨 / 경험치 ───────────────────────────────────────

	// Level: 현재 레벨 (화면 표기는 "LV.")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Level")
	int32 Level = 1;

	// MaxLevel: 최대 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Level", meta = (ClampMin = "1"))
	int32 MaxLevel = 50;

	// CurrentEXP: 현재 레벨 구간에서 쌓인 경험치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Level")
	int32 CurrentEXP = 0;

	// GetEXPToNextLevel: 다음 레벨까지 필요한 경험치. 정확한 성장 곡선은 추후 기획 수치로 교체 예정(임시로 Level*100)
	UFUNCTION(BlueprintCallable, Category = "Stats|Level")
	int32 GetEXPToNextLevel() const;

	// AddExperience: 경험치를 추가하고, 필요한 만큼 자동으로 LevelUp을 반복 호출함
	UFUNCTION(BlueprintCallable, Category = "Stats|Level")
	void AddExperience(int32 Amount);

	// LevelUp: 레벨을 1 올리고 HP/MP 최대치를 갱신 후 전부 회복시킴. 끝나면 OnLevelUp을 브로드캐스트
	UFUNCTION(BlueprintCallable, Category = "Stats|Level")
	void LevelUp();

	// OnLevelUp: 레벨업마다 브로드캐스트되는 델리게이트 (예: AbilityComponent가 스킬 포인트 지급에 사용)
	UPROPERTY(BlueprintAssignable, Category = "Stats|Level")
	FOnCharacterLevelUp OnLevelUp;

	// ─── 기본 스탯 (레벨업으로 증가하지 않음 - 장비/퀘스트 보상/몬스터 처치로만 증가) ───────────────────────────────────────

	// MagicPower: 마력. 기본 공격력으로 데미지 공식에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	int32 MagicPower = 5;

	// Intelligence: 지능. 데미지 공식의 부가 스탯 + 레벨업 시 MP 추가 증가량에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	int32 Intelligence = 0;

	// Vitality: 체력. HP와 방어력에 동시에 기여
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	int32 Vitality = 0;

	// BaseDefense: 체력에서 파생되는 방어력을 제외한 순수 방어력 (장비 등으로 증가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	int32 BaseDefense = 0;

	// ─── 재생력 ───────────────────────────────────────

	// RegenInterval: HP/MP가 자동으로 회복되는 주기(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Regen")
	float RegenInterval = 5.f;

	// ─── HP ───────────────────────────────────────

	// BaseMaxHP: 1레벨 기준 기본 최대 HP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|HP")
	float BaseMaxHP = 50.f;

	// HPPerLevel: 레벨업 1회당 늘어나는 HP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|HP")
	float HPPerLevel = 10.f;

	// HPPerVitality: 체력 1당 늘어나는 HP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|HP")
	float HPPerVitality = 7.3f;

	// EquipmentBonusHP: 장착 중인 장비로부터 얻는 HP 보너스 합계 (장비 시스템에서 갱신)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|HP")
	float EquipmentBonusHP = 0.f;

	// CurrentHP: 현재 체력
	UPROPERTY(BlueprintReadOnly, Category = "Stats|HP")
	float CurrentHP = 0.f;

	// HPRegenAmount: RegenInterval마다 한 번씩 자동으로 회복되는 HP량 (장비/퀘스트 보상 등으로 증가 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|HP")
	float HPRegenAmount = 5.f;

	// GetMaxHP: 현재 레벨·체력·장비 보너스를 반영한 최대 HP
	UFUNCTION(BlueprintCallable, Category = "Stats|HP")
	float GetMaxHP() const;

	// ─── MP ───────────────────────────────────────

	// BaseMaxMP: 1레벨 기준 기본 최대 MP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|MP")
	float BaseMaxMP = 15.f;

	// MPPerLevel: 레벨업 1회당 기본으로 늘어나는 MP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|MP")
	float MPPerLevel = 15.f;

	// IntelligencePerBonusMP: 지능이 이 값만큼 쌓일 때마다 레벨업 시 MP +1 추가
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|MP")
	float IntelligencePerBonusMP = 2.f;

	// EquipmentBonusMP: 장착 중인 장비로부터 얻는 MP 보너스 합계 (장비 시스템에서 갱신)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|MP")
	float EquipmentBonusMP = 0.f;

	// LevelAccumulatedMaxMP: 레벨업 때마다 그 시점의 지능을 반영해 누적되는 최대 MP.
	// 지능 보너스가 "레벨업 시점의 지능"에 좌우되는 경로 의존적 값이라 순수 계산식이 아닌 누적값으로 관리함
	UPROPERTY(BlueprintReadOnly, Category = "Stats|MP")
	float LevelAccumulatedMaxMP = 15.f;

	// CurrentMP: 현재 마나
	UPROPERTY(BlueprintReadOnly, Category = "Stats|MP")
	float CurrentMP = 0.f;

	// MPRegenAmount: RegenInterval마다 한 번씩 자동으로 회복되는 MP량 (장비/퀘스트 보상 등으로 증가 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|MP")
	float MPRegenAmount = 5.f;

	// GetMaxMP: 누적된 레벨업 보너스와 장비 보너스를 반영한 최대 MP
	UFUNCTION(BlueprintCallable, Category = "Stats|MP")
	float GetMaxMP() const;

	// RestoreMP: 지정한 양만큼 MP를 회복시키되 최대치를 넘지 않음 (명상 등 회복 수단에서 사용)
	UFUNCTION(BlueprintCallable, Category = "Stats|MP")
	void RestoreMP(float Amount);

	// SpendMP: MP가 충분하면 차감하고 true 반환, 부족하면 아무 것도 하지 않고 false 반환 (스킬 코스트 등에서 사용)
	UFUNCTION(BlueprintCallable, Category = "Stats|MP")
	bool SpendMP(float Amount);

	// ─── 전투 계산 ───────────────────────────────────────

	// GetTotalDefense: 순수 방어력 + 체력에서 파생된 방어력의 합
	UFUNCTION(BlueprintCallable, Category = "Stats|Combat")
	float GetTotalDefense() const;

	// NearZeroDamageThreshold: (공격력-방어력)이 이 값 이하로 남으면 "잉여 대미지" 보정을 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Combat|Formula")
	float NearZeroDamageThreshold = 5.f;

	// SurplusDamageMin / SurplusDamageMax: 잉여 대미지 보정 시 추가로 더해줄 랜덤 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Combat|Formula")
	float SurplusDamageMin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Combat|Formula")
	float SurplusDamageMax = 5.f;

	// DamageRandomVarianceMax: 최종 대미지에 추가로 곱해지는 랜덤 편차의 최대 비율 (0.2 = 최대 +20%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Combat|Formula")
	float DamageRandomVarianceMax = 0.2f;

	// CalculateIncomingDamage: 공격자의 레벨/마력/지능을 받아 이 캐릭터(방어자)에게 들어오는 최종 피해량을 계산.
	// 공식: ((방어자 레벨×10) - (공격자 레벨×10) + (공격자 마력 + 공격자 지능/2) - 방어자 방어력) × DamageMultiplier
	// DamageMultiplier는 기본공격이면 1.0(100%), 스킬이면 3.5(350%)처럼 스킬별 배율을 넣는 용도
	// 그 결과가 0 이하면 최소 피해 1, 임계값 이하로 작으면 잉여 대미지 보정을 적용
	// 마지막으로 0~DamageRandomVarianceMax 비율만큼 랜덤하게 추가로 곱해줌
	UFUNCTION(BlueprintCallable, Category = "Stats|Combat")
	float CalculateIncomingDamage(int32 AttackerLevel, float AttackerMagicPower, float AttackerIntelligence, float DamageMultiplier = 1.f) const;

	// ApplyDamage: 계산된 최종 대미지를 실제로 CurrentHP에서 차감(0 미만으로는 내려가지 않음).
	// CalculateIncomingDamage()는 수치만 "계산"하고, 실제로 HP를 "적용"하는 건 이 함수의 역할
	UFUNCTION(BlueprintCallable, Category = "Stats|Combat")
	void ApplyDamage(float Amount);

	// ─── 공격속도 ───────────────────────────────────────
	// 나중에 장비/패시브 스킬이 공격속도 보너스를 줄 때를 위한 기반. 몽타주 재생속도·쿨다운·이동 잠금시간 등에
	// GetAttackSpeedMultiplier()를 곱하거나 나눠서 일관되게 반영한다

	// BaseAttackSpeedMultiplier: 기본 공격속도 배율 (1.0 = 정상 속도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|AttackSpeed")
	float BaseAttackSpeedMultiplier = 1.0f;

	// EquipmentBonusAttackSpeed: 장비로부터 얻는 공격속도 보너스 합계
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|AttackSpeed")
	float EquipmentBonusAttackSpeed = 0.f;

	// PassiveBonusAttackSpeed: 패시브 스킬로부터 얻는 공격속도 보너스 합계
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|AttackSpeed")
	float PassiveBonusAttackSpeed = 0.f;

	// GetAttackSpeedMultiplier: 최종 공격속도 배율. 0.1 미만으로는 안 내려가게 클램프(0으로 나누기 방지)
	UFUNCTION(BlueprintCallable, Category = "Stats|AttackSpeed")
	float GetAttackSpeedMultiplier() const;

	// ─── 소환수 파트너 패시브 ───────────────────────────────────────
	// 소환 중인 몬스터들이 주는 보너스. MonsterCompanionComponent가 소환 구성이 바뀔 때마다 전부 지우고 다시 채운다.
	// 저장하지 않음(소환 상태에 따라 매번 다시 계산되는 값)

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusMaxHP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusMaxMP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusDefense = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusHPRegen = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusMPRegen = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusAttackSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|SummonPassive")
	float SummonBonusMoveSpeed = 0.f;

	// ClearSummonPassives: 소환수 패시브 보너스를 전부 0으로 되돌림
	UFUNCTION(BlueprintCallable, Category = "Stats|SummonPassive")
	void ClearSummonPassives();

	// AddSummonPassive: 패시브 종류에 맞는 보너스 항목에 Value를 더함 (같은 종류는 합산)
	UFUNCTION(BlueprintCallable, Category = "Stats|SummonPassive")
	void AddSummonPassive(ESummonPassiveType Type, float Value);

	// GetMoveSpeedMultiplier: 이동속도 배율 (1.0 = 정상 속도). 플레이어 이동 처리에서 MaxWalkSpeed에 곱함
	UFUNCTION(BlueprintCallable, Category = "Stats|SummonPassive")
	float GetMoveSpeedMultiplier() const;

	// ─── 소지금 ───────────────────────────────────────

	// Money: 소지금. 초기값 0
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Economy")
	int64 Money = 0;

	UFUNCTION(BlueprintCallable, Category = "Stats|Economy")
	void AddMoney(int64 Amount);

	// SpendMoney: 소지금이 충분하면 차감하고 true 반환, 부족하면 아무 것도 하지 않고 false 반환
	UFUNCTION(BlueprintCallable, Category = "Stats|Economy")
	bool SpendMoney(int64 Amount);

protected:
	virtual void BeginPlay() override;

	// ApplyRegenTick: RegenInterval마다 호출되어 HPRegenAmount/MPRegenAmount만큼 회복시킴
	void ApplyRegenTick();

private:
	FTimerHandle RegenTimerHandle;
};
