// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/MonsterAIComponent.h"
#include "WildMonsterAIComponent.generated.h"

class AAIController;

// UWildMonsterAIComponent: 필드·던전에 사는 야생 몬스터의 AI.
// 게임이 시작되면(BeginPlay) 그 자리를 "집"으로 기억하고 스스로 움직인다. 플레이어의 소환수로 스폰된 개체는 동작하지 않음.
//   - 감지: 선공 몬스터(bAggressive)는 AggroRadius 안의 적대 진영(플레이어·소환수)을 먼저 공격. 비선공은 맞았을 때만 반격
//   - 추격: 대상과 싸움 (스킬 → 평타 → 접근은 부모 UMonsterAIComponent가 처리)
//   - 귀환: 집에서 LeashRadius 이상 끌려가거나 대상을 잃으면 집으로 돌아가고, 도착하면 HP를 회복
//   - 순찰: 싸울 대상이 없으면 집 주변 PatrolRadius 안을 천천히 돌아다님
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UWildMonsterAIComponent : public UMonsterAIComponent
{
	GENERATED_BODY()

public:
	UWildMonsterAIComponent();

	// bAutoStart: 게임 시작 시 자동으로 AI를 켤지. 끄면 StartWildAI를 직접 호출해야 움직임 (컷신·이벤트 몬스터용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI")
	bool bAutoStart = true;

	// ─── 감지 / 추격 ───────────────────────────────────────

	// bAggressive: true면 감지 범위 안의 적을 먼저 공격(선공), false면 공격받았을 때만 반격(비선공)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Aggro")
	bool bAggressive = true;

	// AggroRadius: 선공 몬스터가 적을 알아차리는 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Aggro")
	float AggroRadius = 900.f;

	// LeashRadius: 집에서 이 거리 이상 끌려가면 싸움을 포기하고 집으로 돌아감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Aggro")
	float LeashRadius = 2000.f;

	// bHealOnReturn: 집으로 돌아가는 데 성공하면 HP를 가득 채움 (끌고 가서 조금씩 깎는 플레이 방지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Aggro")
	bool bHealOnReturn = true;

	// ─── 순찰 ───────────────────────────────────────

	// bPatrol: 싸울 대상이 없을 때 집 주변을 돌아다닐지 (false면 제자리에 서 있음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Patrol")
	bool bPatrol = true;

	// PatrolRadius: 집을 중심으로 이 반경 안에서만 돌아다님
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Patrol")
	float PatrolRadius = 500.f;

	// PatrolPauseMin / PatrolPauseMax: 한 번 이동한 뒤 다음 이동까지 쉬는 시간(초) 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Patrol")
	float PatrolPauseMin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Patrol")
	float PatrolPauseMax = 7.f;

	// PatrolSpeedMultiplier: 순찰할 때는 원래 이동속도에 이 배율을 곱해 천천히 걸음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wild AI|Patrol")
	float PatrolSpeedMultiplier = 0.5f;

	// HomeLocation: AI가 켜진 순간의 위치. 순찰 중심이자 귀환 지점
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Wild AI")
	FVector HomeLocation = FVector::ZeroVector;

	// StartWildAI: 현재 위치를 집으로 삼고 AI를 켬 (플레이어 소환수면 아무것도 안 함)
	UFUNCTION(BlueprintCallable, Category = "Wild AI")
	void StartWildAI();

	// StopWildAI: AI를 끔 (사망, 소환수로 전환 등)
	UFUNCTION(BlueprintCallable, Category = "Wild AI")
	void StopWildAI();

	// NotifyAttacked: 공격받았을 때 BaseMonster::TakeDamage에서 호출. 싸우던 대상이 없으면 공격자를 대상으로 삼음
	void NotifyAttacked(AActor* Attacker);

	// GetCurrentTarget: 지금 싸우고 있는 대상 (없으면 nullptr)
	UFUNCTION(BlueprintCallable, Category = "Wild AI")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

protected:
	virtual void BeginPlay() override;
	virtual void TickDecision() override;

private:
	// FindNearestHostile: AggroRadius 안에서 공격할 수 있는 가장 가까운 대상
	AActor* FindNearestHostile() const;

	// CanTarget: 살아있는 적대 진영이고, (bIgnoresPlayerCharacter 몬스터라면) 플레이어 캐릭터가 아닌지
	bool CanTarget(const AActor* Candidate) const;

	// BeginReturnHome / UpdateReturnHome: 대상을 버리고 집으로 돌아감. 도착하면 (설정에 따라) HP 회복
	void BeginReturnHome();
	void UpdateReturnHome(AAIController* AICon);

	// UpdatePatrol: 집 주변의 갈 수 있는 지점을 골라 천천히 걸어가고, 도착하면 잠깐 쉬었다가 다시 고름
	void UpdatePatrol(AAIController* AICon);

	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	bool bReturningHome = false;

	bool bPatrolWalking = false;
	float NextPatrolTime = 0.f;
};
