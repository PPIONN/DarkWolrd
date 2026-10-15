// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RabbitFaceComponent.generated.h"

// ERabbitExpression: 토끼 얼굴 표정
UENUM(BlueprintType)
enum class ERabbitExpression : uint8
{
	Neutral,
	Happy,      // ^^ 웃는 눈 + 입꼬리 올라감
	Angry,      // 눈꼬리 처진 눈꺼풀 + 찡그린 눈썹
	Sad,        // 눈썹 안쪽이 올라가고 눈이 살짝 내려옴
	Surprised,  // 눈썹이 올라감
	Hurt        // 눈을 질끈 감고 찡그림
};

// URabbitFaceComponent: 토끼 메시의 모프 타깃(셰이프키)으로 표정과 눈 깜빡임을 만듦. ABP 수정 없이 동작.
// - 평소에는 BlinkIntervalMin~Max초마다 무작위로 깜빡임
// - PlayExpression으로 잠깐 표정을 지었다가 기본 표정(SetBaseExpression)으로 돌아옴
// 눈꺼풀은 눈알을 따라 둥글게 움직여야 해서, 완전히 감는 과정을 Blink_L_1~4처럼 단계별 모프로 나눠두고 사이를 이어서 섞음
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API URabbitFaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URabbitFaceComponent();

	// bAutoBlink / BlinkIntervalMin / BlinkIntervalMax / BlinkDuration: 자동 깜빡임 간격(초)과 한 번 깜빡이는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	bool bAutoBlink = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	float BlinkIntervalMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	float BlinkIntervalMax = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	float BlinkDuration = 0.16f;

	// BlendSpeed: 표정이 바뀌는 빠르기 (클수록 빨리 바뀜)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face")
	float BlendSpeed = 12.f;

	// PlayExpression: Duration초 동안 표정을 지은 뒤 기본 표정으로 돌아감 (0 이하면 다른 표정이 올 때까지 유지)
	UFUNCTION(BlueprintCallable, Category = "Face")
	void PlayExpression(ERabbitExpression Expression, float Duration = 1.f);

	// SetBaseExpression: 다른 표정이 없을 때 짓는 기본 표정 (예: 쓰러지면 Sad, 부활하면 Neutral)
	UFUNCTION(BlueprintCallable, Category = "Face")
	void SetBaseExpression(ERabbitExpression Expression);

	UFUNCTION(BlueprintCallable, Category = "Face")
	ERabbitExpression GetCurrentExpression() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 표정 하나를 이루는 값들 (0~1). UpperClose = 윗눈꺼풀이 내려온 정도, HappyEyes = ^^ 웃는 눈
	struct FFaceWeights
	{
		float UpperClose = 0.f;
		float HappyEyes = 0.f;
		float LidAngry = 0.f;
		float BrowAngry = 0.f;
		float BrowSad = 0.f;
		float BrowUp = 0.f;
		float Smile = 0.f;
	};

	static FFaceWeights WeightsFor(ERabbitExpression Expression);

	// UpdateBlink: 자동 깜빡임 진행도(0 = 뜸, 1 = 감음)
	float UpdateBlink(float DeltaTime);

	ERabbitExpression BaseExpression = ERabbitExpression::Neutral;
	ERabbitExpression TimedExpression = ERabbitExpression::Neutral;
	float TimedExpressionEndTime = -1.f;

	FFaceWeights Current;

	float NextBlinkTime = 0.f;
	float BlinkStartTime = -1.f;
};
