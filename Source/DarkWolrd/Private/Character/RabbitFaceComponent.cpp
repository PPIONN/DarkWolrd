// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/RabbitFaceComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	// 모프 타깃 이름 (Blender 셰이프키 이름과 같음). [0] = 왼쪽 눈, [1] = 오른쪽 눈
	const FName BlinkNames[2][4] = {
		{ TEXT("Blink_L_1"), TEXT("Blink_L_2"), TEXT("Blink_L_3"), TEXT("Blink_L_4") },
		{ TEXT("Blink_R_1"), TEXT("Blink_R_2"), TEXT("Blink_R_3"), TEXT("Blink_R_4") } };
	const FName HappyNames[2][3] = {
		{ TEXT("Happy_L_1"), TEXT("Happy_L_2"), TEXT("Happy_L_3") },
		{ TEXT("Happy_R_1"), TEXT("Happy_R_2"), TEXT("Happy_R_3") } };
	const FName LidAngryNames[2] = { TEXT("LidAngry_L"), TEXT("LidAngry_R") };
	const FName BrowAngryName(TEXT("Brow_Angry"));
	const FName BrowSadName(TEXT("Brow_Sad"));
	const FName BrowUpName(TEXT("Brow_Up"));
	const FName SmileName(TEXT("Mouth_Smile"));

	// Value(0~1)를 단계 모프들에 나눠 줌: 인접한 두 단계 사이를 선형으로 이어서, 눈꺼풀이 눈알을 따라 둥글게 움직이게 함
	template <int32 Stages>
	void ApplyStagedMorphs(USkeletalMeshComponent* Mesh, const FName (&Names)[Stages], float Value, float Scale, float ExtraOnLast)
	{
		const float Scaled = FMath::Clamp(Value, 0.f, 1.f) * Stages;
		for (int32 Stage = 1; Stage <= Stages; ++Stage)
		{
			float Weight = FMath::Clamp(1.f - FMath::Abs(Scaled - Stage), 0.f, 1.f) * Scale;
			if (Stage == Stages)
			{
				Weight += ExtraOnLast;
			}
			Mesh->SetMorphTarget(Names[Stage - 1], Weight);
		}
	}
}


URabbitFaceComponent::URabbitFaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void URabbitFaceComponent::BeginPlay()
{
	Super::BeginPlay();
	NextBlinkTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(BlinkIntervalMin, BlinkIntervalMax);
}


void URabbitFaceComponent::PlayExpression(ERabbitExpression Expression, float Duration)
{
	TimedExpression = Expression;
	TimedExpressionEndTime = Duration > 0.f ? GetWorld()->GetTimeSeconds() + Duration : TNumericLimits<float>::Max();
}


void URabbitFaceComponent::SetBaseExpression(ERabbitExpression Expression)
{
	BaseExpression = Expression;
}


ERabbitExpression URabbitFaceComponent::GetCurrentExpression() const
{
	const UWorld* World = GetWorld();
	return (World && World->GetTimeSeconds() < TimedExpressionEndTime) ? TimedExpression : BaseExpression;
}


URabbitFaceComponent::FFaceWeights URabbitFaceComponent::WeightsFor(ERabbitExpression Expression)
{
	FFaceWeights Weights;
	switch (Expression)
	{
	case ERabbitExpression::Happy:
		Weights.HappyEyes = 1.f;
		Weights.Smile = 1.f;
		Weights.BrowUp = 0.35f;
		break;
	case ERabbitExpression::Angry:
		Weights.LidAngry = 1.f;
		Weights.BrowAngry = 1.f;
		break;
	case ERabbitExpression::Sad:
		Weights.UpperClose = 0.2f;
		Weights.BrowSad = 1.f;
		break;
	case ERabbitExpression::Surprised:
		Weights.BrowUp = 1.f;
		break;
	case ERabbitExpression::Hurt:
		Weights.UpperClose = 1.f;
		Weights.BrowAngry = 0.8f;
		break;
	default:
		break;
	}
	return Weights;
}


// 깜빡임 진행도: 빨리 감고 조금 더 천천히 뜸. 가끔은 연달아 두 번 깜빡임
float URabbitFaceComponent::UpdateBlink(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (BlinkStartTime < 0.f)
	{
		if (!bAutoBlink || Now < NextBlinkTime) return 0.f;
		BlinkStartTime = Now;
	}

	const float T = (Now - BlinkStartTime) / FMath::Max(BlinkDuration, 0.01f);
	if (T >= 1.f)
	{
		BlinkStartTime = -1.f;
		NextBlinkTime = Now + (FMath::FRand() < 0.15f ? 0.12f : FMath::FRandRange(BlinkIntervalMin, BlinkIntervalMax));
		return 0.f;
	}
	return T < 0.4f ? T / 0.4f : 1.f - (T - 0.4f) / 0.6f;
}


void URabbitFaceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	if (!Mesh) return;

	const FFaceWeights Target = WeightsFor(GetCurrentExpression());
	const float Alpha = 1.f - FMath::Exp(-BlendSpeed * DeltaTime);
	Current.UpperClose = FMath::Lerp(Current.UpperClose, Target.UpperClose, Alpha);
	Current.HappyEyes = FMath::Lerp(Current.HappyEyes, Target.HappyEyes, Alpha);
	Current.LidAngry = FMath::Lerp(Current.LidAngry, Target.LidAngry, Alpha);
	Current.BrowAngry = FMath::Lerp(Current.BrowAngry, Target.BrowAngry, Alpha);
	Current.BrowSad = FMath::Lerp(Current.BrowSad, Target.BrowSad, Alpha);
	Current.BrowUp = FMath::Lerp(Current.BrowUp, Target.BrowUp, Alpha);
	Current.Smile = FMath::Lerp(Current.Smile, Target.Smile, Alpha);

	// 웃는 눈(^^)이거나 이미 감은 눈이면 깜빡이지 않음
	float Blink = UpdateBlink(DeltaTime);
	if (Current.HappyEyes > 0.05f || Current.UpperClose > 0.9f)
	{
		Blink = 0.f;
	}

	// 윗눈꺼풀: 평소엔 단계 모프로 감고, 화난 눈꺼풀일 때는 그 자리에서 바로 감은 모양으로 섞음
	const float Close = FMath::Max(Current.UpperClose, Blink);
	const float Angry = Current.LidAngry;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		ApplyStagedMorphs(Mesh, BlinkNames[Side], Close, 1.f - Angry, Angry * Blink);
		Mesh->SetMorphTarget(LidAngryNames[Side], Angry * (1.f - Blink));
		ApplyStagedMorphs(Mesh, HappyNames[Side], Current.HappyEyes, 1.f, 0.f);
	}
	Mesh->SetMorphTarget(BrowAngryName, Current.BrowAngry);
	Mesh->SetMorphTarget(BrowSadName, Current.BrowSad);
	Mesh->SetMorphTarget(BrowUpName, Current.BrowUp);
	Mesh->SetMorphTarget(SmileName, Current.Smile);
}
