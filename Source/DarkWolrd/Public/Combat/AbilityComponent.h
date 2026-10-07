// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityComponent.generated.h"

class AMagicOrbProjectile;
class UAnimMontage;
class UCharacterStatComponent;

// EAbilityCastState: 캐릭터 전체가 공유하는 캐스팅 슬롯의 상태.
// 기본공격/스킬 구분 없이 슬롯은 하나뿐이라, 뭔가 준비 중이거나 준비 완료된 상태에서는
// 다른 어빌리티 입력이 들어와도 무시된다.
UENUM(BlueprintType)
enum class EAbilityCastState : uint8
{
	Idle,
	Casting,
	Ready
};

// FAbilityData: 어빌리티(기본공격, 화염 스킬, 물 스킬 등) 하나를 설명하는 데이터.
// 기본공격과 각 스킬은 전부 이 구조체로 정의되어 같은 캐스팅/발사 로직을 공유한다.
USTRUCT(BlueprintType)
struct FAbilityData
{
	GENERATED_BODY()

	// ProjectileClass: 이 어빌리티가 준비할 때 스폰하는 구슬 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSubclassOf<AMagicOrbProjectile> ProjectileClass;

	// BaseOrbCount: 기본 오브 개수 (레벨 보너스가 더해지기 전 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	int32 BaseOrbCount = 1;

	// CastTime: 캐스팅에 걸리는 총 시간(초). 오브들은 이 시간 동안 하나씩 순차적으로 나타난다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float CastTime = 0.6f;

	// OrbRingRadius / OrbRingHeight: 준비된 오브들이 캐릭터 주변에 배치되는 원의 반지름/높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float OrbRingRadius = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float OrbRingHeight = 40.f;

	// CastMontage: 캐스팅 중 재생할 애니메이션 몽타주 (선택 사항)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	UAnimMontage* CastMontage = nullptr;

	// bInstantCast: true면 준비 단계 없이 누르는 즉시 오브를 전부 생성해서 곧바로 발사한다(기본공격에 적합).
	// false면 기존처럼 CastTime 동안 오브를 순차적으로 준비했다가 다시 눌러야 발사된다(스킬에 적합 — "준비하는 자" 컨셉)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bInstantCast = false;

	// Cooldown: 이 어빌리티를 사용한 뒤 다시 쓸 수 있기까지 걸리는 시간(초). 0이면 쿨다운 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Cooldown = 0.f;
};

// UAbilityComponent: 캐릭터에 부착해 사용하는 마법 시전 컴포넌트.
// "준비하는 자"라는 컨셉대로, 기본공격과 스킬 모두 캐스팅(준비)-발사의 2단계 입력으로 동작하며,
// 캐스팅 슬롯은 캐릭터당 하나뿐이라 동시에 두 가지를 준비할 수 없다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

	// TryActivateAbility: 어빌리티 입력 처리.
	// 슬롯이 비어있으면 해당 어빌리티의 캐스팅을 시작하고, 같은 어빌리티가 이미 준비 완료 상태면 발사한다.
	// 캐스팅 중이거나 다른 어빌리티가 준비된 상태에서 다른 키가 들어오면 무시(false 반환)
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryActivateAbility(FName AbilityID);

	// Abilities: 어빌리티 ID("BasicAttack" 등) -> 데이터. 에디터에서 캐릭터 블루프린트마다 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TMap<FName, FAbilityData> Abilities;

	// BonusOrbCountFromLevel: 레벨업 등으로 늘어나는 추가 오브 개수. 모든 어빌리티에 동일하게 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	int32 BonusOrbCountFromLevel = 0;

	// AimTraceDistance: 조준 트레이스(화면 중앙)의 최대 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float AimTraceDistance = 5000.f;

	// MaxAimDownPitch: 조준 트레이스가 아래로 내려가는 각도의 최대치(도 단위, 양수로 입력).
	// 카메라가 캐릭터를 위에서 내려다볼 때 그 각도 그대로 트레이스하면 오브가 바닥에 너무 가파르게 꽂히므로,
	// 일정 각도 이상은 못 내려가게(더 수평에 가깝게) 눌러준다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float MaxAimDownPitch = 15.f;

	// ─── 락온 ───────────────────────────────────────

	// LockedTarget: 락온된 대상. 유효한 동안은 GetAimPoint()가 화면 중앙 트레이스 대신 이 대상을 조준점으로 사용
	UPROPERTY(BlueprintReadOnly, Category = "Ability|LockOn")
	AActor* LockedTarget;

	// LockOnHeightOffset: 락온 대상의 발밑이 아니라 몸통 쪽을 조준하도록 위로 띄우는 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|LockOn")
	float LockOnHeightOffset = 50.f;

	// LockOnRange: 락온할 수 있는 최대 거리. 락온 중 대상이 이 거리의 1.5배보다 멀어지면 자동 해제
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|LockOn")
	float LockOnRange = 2500.f;

	// LockOnMaxAngle: 카메라 정면(화면 중앙)에서 이 각도 안에 있는 몬스터만 락온 후보 (정확히 조준하지 않아도 됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|LockOn")
	float LockOnMaxAngle = 30.f;

	// ToggleLockOn: 이미 락온 중이면 해제. 아니면 화면 중앙에 가장 가까운 적대 몬스터에게 락온
	UFUNCTION(BlueprintCallable, Category = "Ability|LockOn")
	void ToggleLockOn();

	// IsLockedOn: 현재 유효한 락온 대상이 있는지 (HUD 조준 표시 등에 사용 가능)
	UFUNCTION(BlueprintCallable, Category = "Ability|LockOn")
	bool IsLockedOn() const;

	// GetCastState: 현재 캐스팅 슬롯 상태 조회 (UI 표시 등에 사용 가능)
	UFUNCTION(BlueprintCallable, Category = "Ability")
	EAbilityCastState GetCastState() const { return CastState; }

	// GetActiveAbilityID: 현재 캐스팅 중이거나 준비 완료된 어빌리티 ID (없으면 NAME_None)
	UFUNCTION(BlueprintCallable, Category = "Ability")
	FName GetActiveAbilityID() const { return ActiveAbilityID; }

	// ─── 스킬 레벨 / 스킬 포인트 (모든 직업/스킬이 공유하는 시스템) ───────────────────────────────────────
	// 캐릭터 레벨업마다 스킬 포인트를 1개씩 얻고, 스킬창(K 등)에서 원하는 스킬에 투자해 레벨을 올리는 방식.
	// 화염/물/저주 등 어떤 스킬이든 이 하나의 시스템을 그대로 재사용한다.

	// MaxSkillLevel: 스킬 하나가 올라갈 수 있는 최대 레벨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|SkillPoint")
	int32 MaxSkillLevel = 5;

	// AvailableSkillPoints: 아직 투자하지 않고 보유 중인 스킬 포인트
	UPROPERTY(BlueprintReadOnly, Category = "Ability|SkillPoint")
	int32 AvailableSkillPoints = 0;

	// GetSkillLevel: 해당 어빌리티의 현재 스킬 레벨. 한 번도 배우지 않았으면 0
	UFUNCTION(BlueprintCallable, Category = "Ability|SkillPoint")
	int32 GetSkillLevel(FName AbilityID) const { return SkillLevels.FindRef(AbilityID); }

	// TryLevelUpSkill: 스킬 포인트가 있고 최대 레벨에 도달하지 않았으면 해당 스킬 레벨을 1 올리고 포인트 1 소비.
	// 스킬창 UI에서 "+" 버튼을 누르는 동작에 연결하면 됨
	UFUNCTION(BlueprintCallable, Category = "Ability|SkillPoint")
	bool TryLevelUpSkill(FName AbilityID);

	// AddSkillPoints: 스킬 포인트를 지급. 보통 캐릭터 레벨업(OnLevelUp)에 반응해서 호출됨
	UFUNCTION(BlueprintCallable, Category = "Ability|SkillPoint")
	void AddSkillPoints(int32 Amount);

	// ─── 쿨다운 ───────────────────────────────────────

	// IsOnCooldown: 해당 어빌리티가 아직 쿨다운 중인지 확인
	UFUNCTION(BlueprintCallable, Category = "Ability|Cooldown")
	bool IsOnCooldown(FName AbilityID) const;

	// GetRemainingCooldown: 남은 쿨다운 시간(초). 쿨다운 중이 아니면 0 (쿨다운 UI 표시 등에 사용)
	UFUNCTION(BlueprintCallable, Category = "Ability|Cooldown")
	float GetRemainingCooldown(FName AbilityID) const;

protected:
	virtual void BeginPlay() override;

private:
	// HandleCharacterLevelUp: 소유 액터의 CharacterStatComponent::OnLevelUp을 구독해서 스킬 포인트를 1 지급
	UFUNCTION()
	void HandleCharacterLevelUp();

	// SkillLevels: 어빌리티 ID -> 현재 스킬 레벨 (0 = 아직 배우지 않음)
	UPROPERTY()
	TMap<FName, int32> SkillLevels;

	// CooldownEndTimes: 어빌리티 ID -> 쿨다운이 끝나는 월드 시간(초). 그 시간이 지나면 다시 사용 가능
	TMap<FName, float> CooldownEndTimes;

	// CachedStatComponent: 같은 액터의 스탯 컴포넌트. 공격속도 배율을 몽타주 재생속도/쿨다운에 반영할 때 사용
	UPROPERTY()
	UCharacterStatComponent* CachedStatComponent;

	// GetAttackSpeedMultiplier: CachedStatComponent가 있으면 그 배율을, 없으면 1(정상 속도)을 반환
	float GetAttackSpeedMultiplier() const;

	// CastInstantly: 준비 단계 없이 오브를 전부 즉시 생성해서 곧바로 조준 지점으로 발사 (bInstantCast 어빌리티용)
	void CastInstantly(FName AbilityID, const FAbilityData& Data);

	// StartCasting: 캐스팅 슬롯을 점유하고, 오브를 하나씩 순차 스폰하는 타이머를 시작
	void StartCasting(FName AbilityID, const FAbilityData& Data);

	// SpawnNextOrbTick: 캐스팅 중 오브를 한 개 스폰. 목표 개수에 도달하면 Ready 상태로 전환
	void SpawnNextOrbTick();

	// FireReadyAbility: 준비된 오브를 전부 조준 지점을 향해 동시에 발사하고 슬롯을 비움
	void FireReadyAbility();

	// GetOrbRingOffset: 캐릭터 주변 원형 배치에서 Index번째 오브의 상대 위치를 계산
	FVector GetOrbRingOffset(int32 Index, int32 Total, const FAbilityData& Data) const;

	// GetAimPoint: 락온 대상이 있으면 그 위치를, 없으면 카메라 위치에서 카메라 정면(화면 중앙)으로 트레이스한 지점을 반환
	FVector GetAimPoint() const;

	// PerformAimTrace: 화면 중앙 조준 트레이스를 실제로 수행. OutTraceEnd에는 트레이스가 무엇에도 안 맞았을 때의 끝점을 채움
	FHitResult PerformAimTrace(FVector& OutTraceEnd) const;

	// IsValidLockOnTarget: 해당 액터가 락온 가능한 대상인지(살아있는 몬스터인지) 확인
	bool IsValidLockOnTarget(AActor* Target) const;

	// FindLockOnTarget: 화면 중앙 트레이스에 몬스터가 맞으면 그 대상, 아니면 LockOnRange·LockOnMaxAngle 안에서
	// 벽에 가려지지 않은 적대 몬스터 중 화면 중앙에 가장 가까운 대상 (없으면 nullptr)
	AActor* FindLockOnTarget() const;

	EAbilityCastState CastState = EAbilityCastState::Idle;
	FName ActiveAbilityID = NAME_None;
	FAbilityData CurrentAbilityData;

	int32 OrbsSpawnedSoFar = 0;
	int32 TotalOrbsToSpawn = 0;

	UPROPERTY()
	TArray<AMagicOrbProjectile*> PendingOrbs;

	FTimerHandle SpawnTimerHandle;
};
