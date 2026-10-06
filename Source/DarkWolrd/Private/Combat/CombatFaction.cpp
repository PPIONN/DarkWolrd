// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/CombatFaction.h"
#include "Monster/BaseMonster.h"
#include "Stats/CharacterStatComponent.h"

ECombatFaction CombatFaction::GetFaction(const AActor* Actor)
{
	if (!IsValid(Actor)) return ECombatFaction::Neutral;

	if (const ABaseMonster* Monster = Cast<ABaseMonster>(Actor))
	{
		return Monster->Faction;
	}
	if (Actor->FindComponentByClass<UCharacterStatComponent>())
	{
		return ECombatFaction::Player;
	}
	return ECombatFaction::Neutral;
}


bool CombatFaction::IsAlive(const AActor* Actor)
{
	if (!IsValid(Actor)) return false;

	if (const ABaseMonster* Monster = Cast<ABaseMonster>(Actor))
	{
		return Monster->IsAlive();
	}
	if (const UCharacterStatComponent* Stat = Actor->FindComponentByClass<UCharacterStatComponent>())
	{
		return Stat->CurrentHP > 0.f;
	}
	return true;
}


bool CombatFaction::AreHostile(const AActor* A, const AActor* B)
{
	const ECombatFaction FactionA = GetFaction(A);
	const ECombatFaction FactionB = GetFaction(B);
	return FactionA != ECombatFaction::Neutral && FactionB != ECombatFaction::Neutral && FactionA != FactionB;
}
