// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatFaction.generated.h"

// ECombatFaction: 전투 진영. Neutral이 아닌 서로 다른 진영끼리는 적대 관계
UENUM(BlueprintType)
enum class ECombatFaction : uint8
{
	Neutral,		// 전투와 무관한 액터 (벽, NPC, 투사체 등)
	Player,			// 플레이어(토끼)와 플레이어의 소환수
	Wild,			// 필드·던전의 야생 몬스터
	ArenaOpponent	// 몬스터대전의 NPC 상대 팀
};

// CombatFaction: 어떤 액터든 넣어서 진영/생존/적대 여부를 판정하는 도우미
namespace CombatFaction
{
	// GetFaction: 몬스터는 자기 Faction 값, 스탯 컴포넌트를 가진 캐릭터(플레이어)는 Player, 나머지는 Neutral
	DARKWOLRD_API ECombatFaction GetFaction(const AActor* Actor);

	// IsAlive: 몬스터는 HP > 0, 플레이어는 스탯 HP > 0. 그 외 액터는 유효하기만 하면 true
	DARKWOLRD_API bool IsAlive(const AActor* Actor);

	// AreHostile: 둘 다 전투 진영(Neutral 아님)이고 진영이 서로 다르면 true
	DARKWOLRD_API bool AreHostile(const AActor* A, const AActor* B);
}
