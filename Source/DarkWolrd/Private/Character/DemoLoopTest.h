// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Engine/TimerHandle.h"

class ARabbit;
class ABaseMonster;
class UMonsterCompanionComponent;
struct FKey;

// FDemoLoopTest: 설치본을 -DemoLoopTest로 실행하면 튜토리얼을 사람 없이 처음부터 출구까지 플레이해 보는 점검기.
// 처음 하는 사람처럼 안내를 따라 걸어가고, 입력은 전부 실제 키 이벤트로 넣어서 키 매핑과 지형 막힘까지 함께 확인함.
// 단계마다 Saved/Screenshots에 화면을 남기고, 결과는 로그의 [LoopTest] 줄로 남긴 뒤 Esc 두 번으로 게임을 끝냄.
// Shipping 빌드에는 들어가지 않음
class FDemoLoopTest : public TSharedFromThis<FDemoLoopTest>
{
public:
	explicit FDemoLoopTest(ARabbit* InRabbit);

	void Start();

private:
	enum class EStage : uint8 { WalkToAim, ShootTwice, WalkToHunt, KillFirst, PickUpCard, Summon, WalkToBats, WatchSummons, HuntTogether, WalkToExit, Finished };

	void Step();
	void SetStage(EStage NewStage, const TCHAR* ShotName = nullptr);
	void SetRoute(TArray<FVector> NewRoute);
	void Fail(const FString& Reason);

	void PressKey(const FKey& Key) const;
	void HoldKey(const FKey& Key) const;
	void ReleaseKey(const FKey& Key) const;
	void StopMoving() const;
	void TakeShot(const FString& Name) const;
	void LookAt(const FVector& Location) const;

	// WalkTowards: 달리기 키와 W를 누른 채 그쪽을 바라봄. 2초 넘게 막히면 점프, 오래 막히면 실패 처리
	void WalkTowards(const FVector& Location);

	// FollowRoute: Route를 따라 걸음. 끝까지 도착하면 true
	bool FollowRoute();

	// FightNearestMonster: 가장 가까운 야생 몬스터에게 다가가 Q로 락온하고 좌클릭으로 공격
	void FightNearestMonster();

	ABaseMonster* FindWildMonster() const;

	TWeakObjectPtr<ARabbit> Rabbit;
	TWeakObjectPtr<ABaseMonster> Target;
	TArray<FVector> Route;
	int32 RouteIndex = 0;
	FTimerHandle TimerHandle;
	EStage Stage = EStage::WalkToAim;
	float StageTime = 0.f;
	float TotalTime = 0.f;
	float StuckTime = 0.f;
	FVector LastLocation = FVector::ZeroVector;
	float WatchedHP = 0.f;
	int32 StartAttackCount = 0;
	int32 StartKillCount = 0;
	int32 LastGuideStep = -1;
	int32 DeathCount = 0;
	bool bWasDead = false;
};

#endif
