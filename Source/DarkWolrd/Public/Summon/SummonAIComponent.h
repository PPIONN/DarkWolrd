// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SummonAIComponent.generated.h"

class ABaseMonster;
class AAIController;
class UAnimMontage;

// USummonAIComponent: 소환된 몬스터 한 마리의 반자율 전투 AI.
// 모든 ABaseMonster가 기본으로 들고 있지만, 소유자가 플레이어의 소환수로 활성화(Activate)되기 전까지는
// 아무 동작도 하지 않는다(야생 몬스터는 계속 비활성 상태).
//
// 우선순위 결정 규칙(숫자가 작을수록 우선):
//   1  = 자기방어 중이던 적과 이미 교전을 시작한 상태(엔진)
//   2  = 플레이어가 현재 락온 중인 대상
//   3  = 자신을 공격한 적(아직 교전 시작 전)
//   (없음) = 플레이어 뒤를 따라다님. 플레이어가 한동안 가만히 있으면 주변을 천천히 배회함
// 같은 우선순위끼리는 새로 끼어들지 않고 기존에 하던 행동을 그대로 유지한다.
//
// 전투(팰월드 방식 자동 전투): 쿨타임이 돌아온 액티브 스킬(ABaseMonster::ActiveSkills, 배열 앞쪽 우선)을
// 사거리 안이면 먼저 쓰고, 쓸 스킬이 없으면 평타를 친다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API USummonAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USummonAIComponent();

	// AttackRange: 평타 사거리. 이 거리 안에 들어오면 이동을 멈추고 평타를 시도함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI")
	float AttackRange = 200.f;

	// AttackInterval: 평타 사이의 최소 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI")
	float AttackInterval = 1.5f;

	// DecisionInterval: 목표 재판단 주기(초). 매 프레임 계산할 필요는 없어서 일정 간격으로만 재평가함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI")
	float DecisionInterval = 0.25f;

	// ─── 따라다니기 ───────────────────────────────────────

	// FollowDistance: 전투가 없을 때 플레이어 뒤쪽으로 떨어져 서 있는 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Follow")
	float FollowDistance = 220.f;

	// FollowAcceptanceRadius: 자기 자리(FollowDistance 지점)에서 이 거리 안이면 이동하지 않고 대기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Follow")
	float FollowAcceptanceRadius = 80.f;

	// CatchUpDistance / CatchUpSpeedMultiplier: 플레이어와 이 거리 이상 벌어지면 이동속도를 배율만큼 올려서 따라잡음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Follow")
	float CatchUpDistance = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Follow")
	float CatchUpSpeedMultiplier = 1.6f;

	// TeleportDistance: 플레이어와 이 거리 이상 벌어지면 (전투 중이어도) 플레이어 뒤로 순간이동함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Follow")
	float TeleportDistance = 2000.f;

	// FollowSlotIndex: 따라다닐 때 서는 자리 번호 (0 = 바로 뒤, 1/2 = 뒤쪽 양옆). 소환할 때 정해짐
	UPROPERTY(BlueprintReadOnly, Category = "Summon AI|Follow")
	int32 FollowSlotIndex = 0;

	// ─── 대기 중 배회 (플레이어가 가만히 있고 싸울 대상도 없을 때) ───────────────────────────────────────

	// IdleWanderDelay: 플레이어가 아무 입력 없이 멈춰 있은 지 이 시간(초)이 지나면 배회를 시작함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float IdleWanderDelay = 3.f;

	// WanderRadius: 플레이어를 중심으로 이 반경 안에서만 돌아다님
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float WanderRadius = 400.f;

	// WanderMinDistanceFromPlayer: 플레이어와 부딪히지 않도록 이보다 가까운 지점은 고르지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float WanderMinDistanceFromPlayer = 150.f;

	// WanderPauseMin / WanderPauseMax: 한 번 이동한 뒤 다음 이동까지 그 자리에서 쉬는 시간(초) 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float WanderPauseMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float WanderPauseMax = 5.f;

	// WanderSpeedMultiplier: 배회할 때는 원래 이동속도에 이 배율을 곱해 천천히 걸음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon AI|Wander")
	float WanderSpeedMultiplier = 0.45f;

	// ActivateSummonAI: 이 몬스터가 플레이어의 소환수로 전투에 참여함. OwnerPlayer를 등록하고 판단 루프를 시작함
	// (이름에 SummonAI를 붙인 이유: UActorComponent에 이미 다른 시그니처의 Activate()가 있어 이름 충돌 방지)
	UFUNCTION(BlueprintCallable, Category = "Summon AI")
	void ActivateSummonAI(AActor* InOwnerPlayer, int32 InFollowSlotIndex = 0);

	// DeactivateSummonAI: 소환 해제. 모든 우선순위 정보를 지우고 판단 루프를 멈춤
	UFUNCTION(BlueprintCallable, Category = "Summon AI")
	void DeactivateSummonAI();

	// GetOwnerPlayer: 이 소환수를 소환한 플레이어 (비활성 상태면 nullptr)
	AActor* GetOwnerPlayer() const { return OwnerPlayer; }

	// NotifyAttacked: 이 소환수가 Attacker에게 공격받았을 때 호출됨(BaseMonster::TakeDamage에서 호출).
	// 이미 다른 적과 교전(우선순위 1)을 시작한 상태면 무시하고 기존 전투를 계속함
	void NotifyAttacked(AActor* Attacker);

	// CommandGather: "4+1" 커맨드. 모든 우선순위 정보를 초기화하고 플레이어 곁으로 강제로 모이게 함
	UFUNCTION(BlueprintCallable, Category = "Summon AI")
	void CommandGather();

	// CommandAttackPlayerTarget: "4+2" 커맨드. 플레이어가 공격중인 대상을 제외한 모든 우선순위를 초기화함
	UFUNCTION(BlueprintCallable, Category = "Summon AI")
	void CommandAttackPlayerTarget();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// TickDecision: DecisionInterval마다 호출되어 현재 목표를 재판단하고 이동/스킬/평타를 실행함
	void TickDecision();

	// DecideTarget: 우선순위 규칙에 따라 지금 싸워야 할 대상을 고름. 없으면 nullptr(따라다니기 상태)
	AActor* DecideTarget();

	// UpdateFollow: 전투 대상이 없을 때 플레이어 뒤 자기 자리로 이동하고, 많이 벌어졌으면 속도를 올림
	void UpdateFollow(AAIController* AICon);

	// GetFollowSlotLocation: FollowSlotIndex에 해당하는 플레이어 뒤쪽 자리의 월드 위치
	FVector GetFollowSlotLocation() const;

	// IsPlayerIdle: 플레이어가 IdleWanderDelay 이상 입력 없이 멈춰 있는지
	bool IsPlayerIdle() const;

	// UpdateWander: 플레이어 주변의 갈 수 있는 지점을 골라 천천히 걸어가고, 도착하면 잠깐 쉬었다가 다시 고름
	void UpdateWander(AAIController* AICon);

	// ApplySpeedMultiplier: 원래 이동속도 × 배율로 설정 (1 = 원래 속도, 따라잡기/배회에서 사용)
	void ApplySpeedMultiplier(float Multiplier);

	// TeleportToPlayer: 플레이어 뒤 자기 자리로 순간이동. 멀리서 싸우던 자기방어 대상은 포기함
	void TeleportToPlayer();

	// TryUseSkill: 쿨타임이 돌아왔고 사거리 안인 첫 번째 액티브 스킬을 사용. 사용했으면 true
	bool TryUseSkill(AActor* Target, float Distance);

	// StartAttack: 몽타주를 재생하고 HitDelay 뒤에 피해를 줌. 평타와 스킬이 공통으로 사용
	void StartAttack(AActor* Target, float DamageMultiplier, float AreaRadius, UAnimMontage* Montage, float HitDelay);

	// ApplyAttackDamage: 공격력 × 배율만큼 대상(또는 AreaRadius 안의 적 전부)에게 피해를 줌
	void ApplyAttackDamage(AActor* Target, float DamageMultiplier, float AreaRadius);

	// DamageEnemy: 적 하나에게 피해를 주고, 처치했으면 소환주에게 경험치 지급을 요청
	void DamageEnemy(ABaseMonster* Enemy, float Damage);

	// FaceTarget: 대상 쪽으로 몸을 돌림(수평 방향만)
	void FaceTarget(AActor* Target);

	// GetAIController: 소유 액터(ACharacter)의 AIController를 가져옴 (없으면 nullptr)
	AAIController* GetAIController() const;

	bool bIsActive = false;

	UPROPERTY()
	AActor* OwnerPlayer = nullptr;

	// SelfDefenseTarget: 자신을 공격한 적. bSelfDefenseEngaged가 true면 우선순위 1, 아니면 3
	UPROPERTY()
	AActor* SelfDefenseTarget = nullptr;

	bool bSelfDefenseEngaged = false;

	// bForceGather: "4+1" 커맨드로 켜짐. 자신이 공격받기 전까지는 계속 플레이어 곁에 모임
	bool bForceGather = false;

	float LastAttackTime = -100.f;

	// NextActionTime: 평타/스킬 동작이 끝나는 시각. 그 전에는 새 행동을 시작하지 않음
	float NextActionTime = 0.f;

	// SkillReadyTimes: ActiveSkills 각각을 다시 쓸 수 있게 되는 시각 (소환 직후에는 전부 사용 가능)
	TArray<float> SkillReadyTimes;

	// DefaultMaxWalkSpeed: 소환 시점의 원래 이동속도. 따라잡기/배회 배율은 항상 이 값을 기준으로 곱함
	float DefaultMaxWalkSpeed = 0.f;
	float CurrentSpeedMultiplier = 1.f;

	// PlayerStillTime: 플레이어 속도가 거의 0인 상태가 이어진 시간 (넉백 등으로 밀리는 중엔 배회하지 않도록)
	float PlayerStillTime = 0.f;

	// bWandering / NextWanderTime: 배회 중인지, 다음 배회 지점을 고를 시각
	bool bWandering = false;
	float NextWanderTime = 0.f;

	// bWanderWalking: 배회 지점으로 걸어가는 중인지 (도착을 감지해서 그때부터 쉬는 시간을 셈)
	bool bWanderWalking = false;

	FTimerHandle DecisionTimerHandle;
};
