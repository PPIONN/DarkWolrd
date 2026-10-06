// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/BaseMonster.h"
#include "FieldBossMonster.generated.h"

// FBossPhase: HP가 일정 비율 아래로 떨어질 때 보스가 바뀌는 단계 하나 (분노, 광폭화 등)
USTRUCT(BlueprintType)
struct FBossPhase
{
	GENERATED_BODY()

	// HPPercent: HP가 최대치의 이 비율 이하로 떨어지는 순간 이 페이즈로 들어감 (0.5 = 50%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float HPPercent = 0.5f;

	// AttackPowerMultiplier: 공격력 배율. 앞 페이즈의 배율에 누적해서 곱해짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPowerMultiplier = 1.25f;

	// AttackSpeedMultiplier: 평타 간격이 이 배율만큼 짧아짐 (1.5 = 1.5배 빠르게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeedMultiplier = 1.f;

	// MoveSpeedMultiplier: 이동속도 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeedMultiplier = 1.f;

	// AddedSkills: 이 페이즈부터 추가로 쓰기 시작하는 스킬 (기존 스킬 뒤에 붙어서 우선순위는 낮음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSummonActiveSkill> AddedSkills;

	// EnterMontage: 페이즈에 들어갈 때 재생할 연출 몽타주 (포효 등). 재생하는 동안 보스는 공격하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* EnterMontage = nullptr;

	// EnterAnnouncement: 페이즈에 들어갈 때 띄울 알림. {Boss}는 보스 이름으로 바뀜 (비우면 알림 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText EnterAnnouncement;

	// MinionClass / MinionCount / MinionSpawnRadius: 페이즈에 들어갈 때 보스 주변에 불러낼 부하 몬스터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MinionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 MinionCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinionSpawnRadius = 500.f;
};

// AFieldBossMonster: 필드보스. 일반 몬스터와 같은 AI로 싸우지만 HP 비율에 따라 페이즈가 바뀌고,
// 가까이 가면 화면 위에 보스 HP바가 뜬다(HUD에 BossPanel/BossNameText/BossHPBar 배치 시).
// 보통 AFieldBossSpawner가 정해진 후보 지점 중 한 곳에 주기적으로 스폰함
UCLASS()
class DARKWOLRD_API AFieldBossMonster : public ABaseMonster
{
	GENERATED_BODY()

public:
	AFieldBossMonster();

	// BossName: 알림과 HP바에 표시할 이름 (비우면 클래스 이름)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	FText BossName;

	// Phases: HP 비율에 따른 페이즈 목록. 순서는 상관없고 HPPercent가 높은 것부터 차례로 들어감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	TArray<FBossPhase> Phases;

	// HUDShowDistance: 플레이어가 이 거리 안에 있으면 HUD에 보스 HP바를 띄움
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	float HUDShowDistance = 3000.f;

	// bRemoveMinionsOnDeath: 보스가 쓰러지면 불러낸 부하도 함께 사라지게 할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field Boss")
	bool bRemoveMinionsOnDeath = true;

	// GetBossDisplayName: 표시용 이름 (BossName이 비어 있으면 클래스 이름)
	UFUNCTION(BlueprintCallable, Category = "Field Boss")
	FText GetBossDisplayName() const;

	// GetEnteredPhaseCount: 지금까지 들어간 페이즈 수 (0 = 아직 기본 상태)
	UFUNCTION(BlueprintCallable, Category = "Field Boss")
	int32 GetEnteredPhaseCount() const { return EnteredPhaseCount; }

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Die() override;

private:
	// CheckPhaseTransitions: 현재 HP 비율에 맞는 페이즈 중 아직 안 들어간 것을 차례로 적용
	void CheckPhaseTransitions();

	// EnterPhase: 강화 배율·추가 스킬·연출·알림·부하 소환을 적용
	void EnterPhase(const FBossPhase& Phase);

	// EnteredPhaseCount: HPPercent 높은 순으로 정렬했을 때 몇 번째 페이즈까지 들어갔는지
	int32 EnteredPhaseCount = 0;

	// SpawnedMinions: 페이즈에서 불러낸 부하들 (보스 사망 시 정리용)
	TArray<TWeakObjectPtr<ABaseMonster>> SpawnedMinions;
};
