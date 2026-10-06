// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/CurseOrbProjectile.h"
#include "Monster/BaseMonster.h"

// 적중 시 저주 적용: 스킬 레벨에 해당하는 증폭률/지속시간 값을 찾아 대상 몬스터에게 ApplyCurse를 호출.
// 배열 범위를 넘는 레벨이면 마지막(가장 강한) 값으로 취급
void ACurseOrbProjectile::ApplyOnHitEffect_Implementation(AActor* HitActor)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(HitActor);
	if (!Monster || AmplifyPercentPerLevel.Num() == 0 || DurationPerLevel.Num() == 0) return;

	const int32 SkillLevel = GetCasterSkillLevel();
	const int32 AmplifyIndex = FMath::Clamp(SkillLevel - 1, 0, AmplifyPercentPerLevel.Num() - 1);
	const int32 DurationIndex = FMath::Clamp(SkillLevel - 1, 0, DurationPerLevel.Num() - 1);

	Monster->ApplyCurse(AmplifyPercentPerLevel[AmplifyIndex], DurationPerLevel[DurationIndex]);
}
