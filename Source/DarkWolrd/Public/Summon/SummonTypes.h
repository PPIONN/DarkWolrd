// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SummonTypes.generated.h"

class UAnimMontage;
class UNiagaraSystem;

// ESkillAreaCenter: 범위 스킬(AreaRadius > 0)의 중심을 어디로 잡을지
UENUM(BlueprintType)
enum class ESkillAreaCenter : uint8
{
	Target,					// 타격 순간 대상이 있는 곳 (피할 수 없음. 소환수·일반 몬스터용)
	TargetLocationAtCast,	// 스킬을 시작한 순간 대상이 서 있던 곳 (HitDelay 안에 빠져나가면 피할 수 있음. 보스 패턴용)
	Self					// 시전자 주변 (내려찍기·포효 등)
};

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

// FSummonActiveSkill: 몬스터가 자동 전투 중 쿨타임이 돌아올 때마다 알아서 쓰는 액티브 스킬 하나.
// 처음엔 소환수용으로 만들어 이름에 Summon이 붙어 있지만, 야생 몬스터·필드보스도 같은 구조를 그대로 씀
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

	// AreaRadius: 0이면 대상 하나만, 0보다 크면 AreaCenter 주변 이 반경 안의 적 전부에게 피해
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AreaRadius = 0.f;

	// AreaCenter: 범위 스킬의 중심. 보스 패턴처럼 피할 수 있게 하려면 TargetLocationAtCast나 Self로 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESkillAreaCenter AreaCenter = ESkillAreaCenter::Target;

	// TelegraphVFX: 스킬 시작부터 타격 순간까지 범위 중심 바닥에 띄울 예고 이펙트(빨간 원 등).
	// 반지름 1m(100) 크기로 만든 이펙트를 AreaRadius에 맞춰 키워서 표시함
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* TelegraphVFX = nullptr;

	// Montage: 사용할 때 재생할 애니메이션 몽타주 (없으면 애니메이션 없이 피해만 들어감)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	// HitDelay: 사용 시작 후 실제로 피해가 들어가는 시점(초). 몽타주의 타격 프레임에 맞춰 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HitDelay = 0.4f;
};
