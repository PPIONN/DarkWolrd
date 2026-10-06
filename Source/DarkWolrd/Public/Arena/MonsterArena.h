// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterArena.generated.h"

class USphereComponent;
class UBillboardComponent;
class ABaseMonster;
class ARabbit;

// FArenaOpponentMonster: NPC 상대 덱의 몬스터 한 마리
USTRUCT(BlueprintType)
struct FArenaOpponentMonster
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 Level = 1;

	// UpgradeCount: 플레이어 카드 중첩 등록과 같은 등급 배율 (1회당 스탯 +15%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 UpgradeCount = 0;
};

// FArenaOpponent: 몬스터대전 상대 한 명 (사다리 한 단계)
USTRUCT(BlueprintType)
struct FArenaOpponent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText OpponentName;

	// Monsters: 상대 덱 (3마리까지 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArenaOpponentMonster> Monsters;

	// 승리 보상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 RewardEXP = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int64 RewardMoney = 200;

	// RewardCard / RewardCardChance: 승리 시 이 확률로 받는 몬스터 카드
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> RewardCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float RewardCardChance = 0.f;
};

UENUM(BlueprintType)
enum class EArenaState : uint8
{
	Idle,		// 대기 (시작 발판을 밟으면 시작)
	Countdown,	// 상대 등장, 카운트다운 중
	Fighting	// 대전 중
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArenaMatchEnded, bool, bPlayerWon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaMatchStarted);

// AMonsterArena: 몬스터대전 경기장. 내 소환수(최대 3)와 NPC 상대 덱이 자동으로 싸우고, 플레이어는 명령(4+1/4+2, 락온)만 내림.
//   1) 플레이어가 ChallengeTrigger(시작 발판)를 밟으면 현재 단계 상대가 반대편에 나타나고 카운트다운 시작
//   2) "시작!"과 함께 내 소환수가 HP 가득 찬 채로 내 쪽 자리로 이동하고 양쪽이 자동으로 교전
//      (대전 중 플레이어는 피해를 받지도 주지도 않고, Y 재소환은 막힘)
//   3) 상대 전멸 = 승리(보상, 다음 단계로), 내 소환수 전멸 = 패배. 제한 시간이 지나면 남은 HP 비율로 판정,
//      플레이어가 경기장을 멀리 벗어나면 기권 패배
UCLASS()
class DARKWOLRD_API AMonsterArena : public AActor
{
	GENERATED_BODY()

public:
	AMonsterArena();

	// ChallengeTrigger: 밟으면 대전이 시작되는 발판 (위치는 경기장 입구 등으로 옮겨서 사용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	USphereComponent* ChallengeTrigger;

	// Ladder: 차례로 상대할 NPC 목록. 이기면 다음 상대로 넘어가고, 마지막 상대를 이긴 뒤에는 그 상대와 재대전
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	TArray<FArenaOpponent> Ladder;

	// CurrentRank: 지금 도전할 사다리 단계 (0부터). 레벨을 다시 열면 처음부터 시작함
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Arena")
	int32 CurrentRank = 0;

	// PlayerSideSpots / OpponentSpots: 내 소환수·상대가 설 자리(Target Point 등). 비워두면 경기장 중심 기준 앞(상대)/뒤(나)로 자동 배치
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Arena")
	TArray<AActor*> PlayerSideSpots;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Arena")
	TArray<AActor*> OpponentSpots;

	// ArenaRadius: 경기장 반지름 (이 액터 위치가 중심). 자동 배치 간격에 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	float ArenaRadius = 1200.f;

	// LeaveDistance: 대전 중 플레이어가 중심에서 이 거리 넘게 벗어나면 기권 패배
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	float LeaveDistance = 3000.f;

	// TimeLimit: 제한 시간(초). 지나면 양쪽 남은 HP 비율로 승패를 가림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena", meta = (ClampMin = "10"))
	float TimeLimit = 180.f;

	// CountdownSeconds: 상대 등장 후 시작까지 세는 초
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena", meta = (ClampMin = "0"))
	int32 CountdownSeconds = 3;

	// RematchCooldown: 대전이 끝난 뒤 다음 대전을 시작할 수 있을 때까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena", meta = (ClampMin = "0"))
	float RematchCooldown = 3.f;

	// 알림 문구: {Rank} 단계, {Opponent} 상대 이름, {Reward} 보상 요약, {Reason} 판정 사유
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Announcement")
	FText StartAnnouncement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Announcement")
	FText WinAnnouncement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Announcement")
	FText LoseAnnouncement;

	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FOnArenaMatchStarted OnMatchStarted;

	UPROPERTY(BlueprintAssignable, Category = "Arena")
	FOnArenaMatchEnded OnMatchEnded;

	// StartMatch: 현재 단계 상대와 대전 시작 (소환수가 없거나 이미 진행 중이면 false)
	UFUNCTION(BlueprintCallable, Category = "Arena")
	bool StartMatch(ARabbit* InChallenger);

	// ForfeitMatch: 진행 중인 대전을 기권(패배) 처리
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void ForfeitMatch();

	UFUNCTION(BlueprintCallable, Category = "Arena")
	EArenaState GetArenaState() const { return ArenaState; }

	// GetTimeRemaining: 대전 남은 시간(초). 대전 중이 아니면 0
	UFUNCTION(BlueprintCallable, Category = "Arena")
	float GetTimeRemaining() const;

	// GetCurrentOpponentName: 지금 도전할 상대 이름 (사다리가 비어 있으면 빈 텍스트)
	UFUNCTION(BlueprintCallable, Category = "Arena")
	FText GetCurrentOpponentName() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleCombatantDied(ABaseMonster* DeadMonster);

	// CountdownTick: 1초마다 남은 숫자를 알리고 0이 되면 BeginFight
	void CountdownTick();

	// BeginFight: 내 소환수를 자리로 옮겨 HP를 채우고 자동 교전 켜기, 상대 AI 켜기, 플레이어 무적·재소환 잠금
	void BeginFight();

	// CheckMatchState: 0.5초마다 승패·제한 시간·이탈을 확인
	void CheckMatchState();

	// EndMatch: 결과 처리(보상, 단계 상승), 남은 상대 정리, 플레이어·소환수 상태 원복, 알림
	void EndMatch(bool bPlayerWon, const FText& Reason);

	void SpawnOpponents();

	// GetSpotLocation: 지정한 자리(없으면 자동 배치 위치)의 바닥 좌표
	FVector GetSpotLocation(bool bOpponentSide, int32 Index, int32 Count) const;

	// GetTeamHealthRatio: 팀 시작 인원 대비 남은 HP 비율의 평균 (쓰러진 개체는 0)
	static float GetTeamHealthRatio(const TArray<TWeakObjectPtr<ABaseMonster>>& Team);

	static int32 CountAlive(const TArray<TWeakObjectPtr<ABaseMonster>>& Team);

	const FArenaOpponent* GetCurrentOpponent() const;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	UBillboardComponent* Billboard;

	EArenaState ArenaState = EArenaState::Idle;

	TWeakObjectPtr<ARabbit> Challenger;

	UPROPERTY()
	TArray<TWeakObjectPtr<ABaseMonster>> PlayerTeam;

	UPROPERTY()
	TArray<TWeakObjectPtr<ABaseMonster>> OpponentTeam;

	int32 CountdownRemaining = 0;
	float FightStartTime = 0.f;
	float NextAllowedStartTime = 0.f;

	FTimerHandle CountdownTimerHandle;
	FTimerHandle CheckTimerHandle;
};
