// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/MagicOrbProjectile.h"
#include "CurseOrbProjectile.generated.h"

// ACurseOrbProjectile: "저주하는 자" 직업의 저주 스킬 오브.
// 적중하면 데미지를 준 뒤, 대상에게 저주(받는 모든 대미지 증폭)를 건다.
// 증폭 비율/지속시간은 캐스터의 이 스킬 레벨(1~5)에 따라 커진다 — 배열의 인덱스 0이 레벨 1에 대응.
UCLASS()
class DARKWOLRD_API ACurseOrbProjectile : public AMagicOrbProjectile
{
	GENERATED_BODY()

public:
	// AmplifyPercentPerLevel: 스킬 레벨별 대미지 증폭 비율(%). 기본값은 레벨 1~5 예시 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curse")
	TArray<float> AmplifyPercentPerLevel = { 20.f, 30.f, 40.f, 50.f, 60.f };

	// DurationPerLevel: 스킬 레벨별 저주 지속시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curse")
	TArray<float> DurationPerLevel = { 3.f, 4.f, 5.f, 6.f, 7.f };

protected:
	// ApplyOnHitEffect_Implementation: 적중 대상이 몬스터면 현재 스킬 레벨에 맞는 증폭률/지속시간으로 저주를 건다
	virtual void ApplyOnHitEffect_Implementation(AActor* HitActor) override;
};
