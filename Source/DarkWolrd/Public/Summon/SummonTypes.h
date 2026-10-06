// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SummonTypes.generated.h"

class UAnimMontage;

// ESummonPassiveType: 소환 중인 몬스터가 플레이어에게 주는 파트너 패시브 종류.
// 팰월드의 파트너 스킬을 "소환해두기만 하면 적용되는 추가 패시브"로 단순화한 것
UENUM(BlueprintType)
enum class ESummonPassiveType : uint8
{
	None,
	MaxHP,			// 최대 HP +Value
	MaxMP,			// 최대 MP +Value
	Defense,		// 방어력 +Value
	HPRegen,		// 재생 주기마다 HP +Value 추가 회복
	MPRegen,		// 재생 주기마다 MP +Value 추가 회복
	AttackSpeed,	// 공격속도 배율 +Value (0.1 = +10%)
	MoveSpeed		// 이동속도 배율 +Value (0.1 = +10%)
};

// FSummonActiveSkill: 소환수가 자동 전투 중 쿨타임이 돌아올 때마다 알아서 쓰는 액티브 스킬 하나
USTRUCT(BlueprintType)
struct FSummonActiveSkill
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SkillName;

	// DamageMultiplier: 몬스터 공격력에 곱해지는 배율 (2.0 = 공격력의 200%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageMultiplier = 2.f;

	// Cooldown: 한 번 쓴 뒤 다시 쓸 수 있을 때까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 8.f;

	// Range: 대상이 이 거리 안에 있어야 사용함. 평타 사거리보다 길면 다가가지 않고 그 자리에서 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Range = 300.f;

	// AreaRadius: 0이면 대상 하나만, 0보다 크면 대상 주변 이 반경 안의 적 전부에게 피해
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AreaRadius = 0.f;

	// Montage: 사용할 때 재생할 애니메이션 몽타주 (없으면 애니메이션 없이 피해만 들어감)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	// HitDelay: 사용 시작 후 실제로 피해가 들어가는 시점(초). 몽타주의 타격 프레임에 맞춰 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HitDelay = 0.4f;
};
