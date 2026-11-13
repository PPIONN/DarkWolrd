// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Containers/Ticker.h"

class ARabbit;
class ABaseMonster;
class UMonsterCompanionComponent;
struct FKey;

// FDemoLoopTest: 설치본을 -DemoLoopTest로 실행하면 데모 한 판을 사람 없이 처음부터 끝까지 플레이해 보는 점검기.
// 시작 화면 → 일부러 한 번 쓰러져 실패 화면과 '마지막 목표에서 다시' 확인 → 튜토리얼을 출구까지 → 클리어 화면 →
// '처음부터 다시'로 시작 화면이 다시 뜨는지까지 확인함. 걷기·공격·소환은 실제 키 이벤트로 넣어 키 매핑과 지형 막힘도 함께 확인.
// 화면 버튼은 버튼이 부르는 함수를 직접 호출함. 단계마다 Saved/Screenshots에 화면을 남기고, 결과는 로그의 [LoopTest] 줄로 남김.
// 화면이 떠 있는 동안 게임이 멈추므로 월드 타이머 대신 엔진 티커로 돌아감. Shipping 빌드에는 들어가지 않음
class FDemoLoopTest : public TSharedFromThis<FDemoLoopTest>
{
public:
	explicit FDemoLoopTest(ARabbit* InRabbit);

	void Start();

	// AttachToRabbit: 맵을 다시 불러온 뒤 새로 생긴 토끼로 이어서 점검
	void AttachToRabbit(ARabbit* NewRabbit);

private:
	enum class EStage : uint8
	{
		Title, FailTest, WalkToAim, ShootTwice, WalkToHunt, KillFirst, PickUpCard, Summon, WalkToBats, WatchSummons, HuntTogether, WalkToExit,
		WaitClear, WaitRestart, AfterRestart, Finished
	};

	bool Tick(float DeltaTime);
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
	FTSTicker::FDelegateHandle TickerHandle;
	EStage Stage = EStage::Title;
	float StageTime = 0.f;
	float TotalTime = 0.f;
	float StuckTime = 0.f;
	float WaitTime = 0.f;
	FVector LastLocation = FVector::ZeroVector;
	float WatchedHP = 0.f;
	int32 StartAttackCount = 0;
	int32 StartKillCount = 0;
	int32 LastGuideStep = -1;
	int32 DeathCount = 0;
	bool bWasDead = false;
	bool bShotTaken = false;
	bool bFailTestDied = false;
	bool bQuitRequested = false;
};

#endif
