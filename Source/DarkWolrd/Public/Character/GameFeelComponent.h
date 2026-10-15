// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "GameFeelComponent.generated.h"

class UNiagaraSystem;
class USoundBase;
class USceneComponent;

// UGameFeelComponent: 애니메이션 위에 덧입히는 "쫀득한" 연출 (토끼·몬스터 공용).
// - 늘어남·찌그러짐: 점프하면 길쭉, 착지·피격하면 납작해졌다가 스프링처럼 출렁이며 돌아옴 (부피는 유지)
// - 기울기: 가속·방향 전환 방향으로 몸이 기울었다가 출렁이며 돌아옴
// - 히트스톱: 맞는 순간 이 액터만 아주 잠깐 멈춤 (CustomTimeDilation)
// 메시의 상대 크기·회전에 곱해서 적용하므로 애니메이션·ABP와 충돌하지 않음. 발밑(메시 원점) 기준으로 변형됨
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UGameFeelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGameFeelComponent();

	// ─── 늘어남·찌그러짐 ───────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	bool bEnableSquash = true;

	// SquashFrequency / SquashDampingRatio: 출렁임 속도(Hz)와 감쇠. 감쇠가 작을수록 더 여러 번 출렁임 (1이면 출렁임 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float SquashFrequency = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float SquashDampingRatio = 0.3f;

	// MaxSquash: 찌그러짐 한계 (0.4 = 키가 최대 40% 줄거나 늘어남)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float MaxSquash = 0.4f;

	// JumpStretch: 점프 순간 위로 늘어나는 정도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float JumpStretch = 0.18f;

	// LandSquashMin / LandSquashMax: 착지 시 납작해지는 정도. 낙하 속도가 LandSpeedForMaxSquash에 가까울수록 Max
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float LandSquashMin = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float LandSquashMax = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float LandSpeedForMaxSquash = 1200.f;

	// LandSpeedThreshold: 이보다 느리게 떨어지면(턱을 살짝 내려오는 정도) 착지 연출을 하지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float LandSpeedThreshold = 250.f;

	// HitSquash: 맞았을 때 납작해지는 정도. 쓰러지는 공격이면 KillSquashMultiplier배
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float HitSquash = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Squash")
	float KillSquashMultiplier = 1.6f;

	// ─── 기울기 ───────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	bool bEnableLean = true;

	// MaxLeanRoll / MaxLeanPitch: 좌우 / 앞뒤 최대 기울기(도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	float MaxLeanRoll = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	float MaxLeanPitch = 7.f;

	// AccelForMaxLean: 이 가속도(cm/s²)에서 최대로 기울어짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	float AccelForMaxLean = 4000.f;

	// LeanFrequency / LeanDampingRatio: 기울기가 따라가는 속도(Hz)와 감쇠 (멈출 때 살짝 뒤로 젖혔다 돌아오는 정도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	float LeanFrequency = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Lean")
	float LeanDampingRatio = 0.5f;

	// ─── 히트스톱 ───────────────────────────────────────

	// HitStopLight / HitStopHeavy / HitStopKill: 약한 타격(소환수끼리) / 강한 타격(플레이어가 때리거나 맞음) / 처치 시 멈추는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Hit Stop")
	float HitStopLight = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Hit Stop")
	float HitStopHeavy = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Hit Stop")
	float HitStopKill = 0.13f;

	// HitStopTimeScale: 멈춰 있는 동안의 시간 배율 (0에 가까울수록 완전히 멈춤)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Hit Stop")
	float HitStopTimeScale = 0.02f;

	// ─── 착지 효과 (선택) ───────────────────────────────────────

	// LandVFX / LandSound: 착지 연출이 나올 때 발밑에 재생 (먼지 등). 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Land FX")
	UNiagaraSystem* LandVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Feel|Land FX")
	USoundBase* LandSound = nullptr;

	// AddSquash: 양수면 납작하게, 음수면 길쭉하게 즉시 변형한 뒤 스프링처럼 돌아옴
	UFUNCTION(BlueprintCallable, Category = "Game Feel")
	void AddSquash(float Amount);

	// HitStop: 이 액터만 Duration초 동안 거의 멈춤. 이미 멈춰 있으면 더 긴 쪽으로 맞춤
	UFUNCTION(BlueprintCallable, Category = "Game Feel")
	void HitStop(float Duration);

	// PlayHitReaction: 맞았을 때 찌그러짐 + 히트스톱. bHeavy = 플레이어가 관련된 타격, bKilled = 이 타격으로 쓰러짐
	UFUNCTION(BlueprintCallable, Category = "Game Feel")
	void PlayHitReaction(bool bHeavy, bool bKilled);

	// SetAppearScale: 소환 등장 연출처럼 몸 전체 크기를 따로 키우고 줄일 때 사용 (1 = 원래 크기)
	void SetAppearScale(float Scale);

	// GetLastFallSpeed: 마지막으로 공중에 있던 프레임의 낙하 속도(cm/s, 양수)
	float GetLastFallSpeed() const { return LastFallSpeed; }

	// StepSpring: Value를 Target 쪽으로 스프링처럼 움직임 (감쇠비 1 미만이면 넘쳤다가 돌아옴). 카메라 연출 등에서도 사용
	static void StepSpring(float& Value, float& Velocity, float Target, float Frequency, float DampingRatio, float DeltaTime);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// UpdateAirState: 점프 시작·착지를 감지해서 늘어남·찌그러짐을 줌
	void UpdateAirState();

	// UpdateLean: 실제 가속도로 기울기 목표를 정하고 스프링으로 따라감
	void UpdateLean(float DeltaTime);

	// ApplyToMesh: 현재 값으로 메시의 상대 크기·회전을 설정
	void ApplyToMesh();

	UPROPERTY()
	USceneComponent* Mesh = nullptr;

	FVector BaseScale = FVector::OneVector;
	FQuat BaseRotation = FQuat::Identity;

	float Squash = 0.f;
	float SquashVelocity = 0.f;
	float LeanRight = 0.f;
	float LeanRightVelocity = 0.f;
	float LeanForward = 0.f;
	float LeanForwardVelocity = 0.f;
	float AppearScale = 1.f;

	FVector LastVelocity = FVector::ZeroVector;
	float LastFallSpeed = 0.f;
	bool bWasFalling = false;
	bool bAtRest = true;

	FTimerHandle HitStopTimerHandle;
};
