// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MonsterAIComponent.generated.h"

class ABaseMonster;
class AAIController;
class UAnimMontage;

// UMonsterAIComponent: 몬스터 AI의 공통 부모.
// 판단 루프(DecisionInterval마다 TickDecision), 대상과 싸우는 방법(쿨타임 돌아온 스킬 → 평타 → 다가가기),
// 피해 적용(진영이 적대인 대상에게만), 이동속도 배율을 담당한다.
// "누구와 싸울지"와 "싸울 대상이 없을 때 무엇을 할지"는 자식 클래스가 정한다.
//   - USummonAIComponent      : 플레이어 소환수 (우선순위, 따라다니기, 배회)
//   - UWildMonsterAIComponent : 야생 몬스터 (감지, 추격, 귀환, 순찰)
UCLASS(Abstract, ClassGroup = (Custom))
class DARKWOLRD_API UMonsterAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterAIComponent();

	// AttackRange: 평타 사거리. 두 몸체 가장자리 사이 거리(캡슐 반지름 제외) 기준이라 덩치가 큰 보스에게도 똑같이 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
	float AttackRange = 120.f;

	// AttackInterval: 평타 사이의 최소 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
	float AttackInterval = 1.5f;

	// DecisionInterval: 상황 재판단 주기(초). 매 프레임 계산할 필요는 없어서 일정 간격으로만 재평가함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster AI")
	float DecisionInterval = 0.25f;

	// IsAIActive: 판단 루프가 돌고 있는지
	UFUNCTION(BlueprintCallable, Category = "Monster AI")
	bool IsAIActive() const { return bIsActive; }

	// GetDistanceToTarget: 두 몸체 가장자리 사이의 수평 거리 (AttackRange, 스킬 Range 판정에 쓰는 값)
	float GetDistanceToTarget(const AActor* Target) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// StartDecisionLoop / StopDecisionLoop: 판단 루프 시작·정지. 시작할 때 스킬 쿨타임과 원래 이동속도를 초기화함
	void StartDecisionLoop();
	void StopDecisionLoop();

	// TickDecision: DecisionInterval마다 호출. 자식 클래스가 무엇을 할지 구현
	virtual void TickDecision() {}

	// EngageTarget: 대상과 싸움. 공격 동작 중이면 기다리고, 쓸 수 있는 스킬 → 평타 → 다가가기 순으로 시도
	void EngageTarget(AActor* Target);

	// OnTargetKilled: 이 몬스터의 공격으로 대상이 쓰러졌을 때 호출 (VictimRewardEXP는 몬스터를 쓰러뜨렸을 때만 0보다 큼)
	virtual void OnTargetKilled(AActor* Victim, float VictimRewardEXP) {}

	// FaceTarget: 대상 쪽으로 몸을 돌림(수평 방향만)
	void FaceTarget(AActor* Target);

	// ApplySpeedMultiplier: 원래 이동속도 × 배율로 설정 (1 = 원래 속도)
	void ApplySpeedMultiplier(float Multiplier);

	AAIController* GetAIController() const;
	ABaseMonster* GetOwnerMonster() const;

	bool bIsActive = false;

private:
	// TryUseSkill: 쿨타임이 돌아왔고 사거리 안인 첫 번째 액티브 스킬을 사용. 사용했으면 true
	bool TryUseSkill(AActor* Target, float Distance);

	// StartAttack: 몽타주를 재생하고 HitDelay 뒤에 피해를 줌. 평타와 스킬이 공통으로 사용
	void StartAttack(AActor* Target, float DamageMultiplier, float AreaRadius, UAnimMontage* Montage, float HitDelay);

	// ApplyAttackDamage: 대상 하나(AreaRadius 0) 또는 대상 주변 반경 안의 적대 진영 전부에게 피해
	void ApplyAttackDamage(AActor* Target, float DamageMultiplier, float AreaRadius);

	// DamageTarget: 적대 대상 하나에게 피해. 플레이어에게는 몬스터→플레이어 공식, 몬스터에게는 공격력 × 배율
	void DamageTarget(AActor* Target, float DamageMultiplier);

	float LastAttackTime = -100.f;

	// NextActionTime: 평타/스킬 동작이 끝나는 시각. 그 전에는 새 행동을 시작하지 않음
	float NextActionTime = 0.f;

	// SkillReadyTimes: ActiveSkills 각각을 다시 쓸 수 있게 되는 시각 (루프 시작 직후에는 전부 사용 가능)
	TArray<float> SkillReadyTimes;

	// DefaultMaxWalkSpeed: 루프 시작 시점의 원래 이동속도. 속도 배율은 항상 이 값을 기준으로 곱함
	float DefaultMaxWalkSpeed = 0.f;
	float CurrentSpeedMultiplier = 1.f;

	FTimerHandle DecisionTimerHandle;
};
