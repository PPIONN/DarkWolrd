// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MonsterCardData.generated.h"

class ABaseMonster;

// FMonsterCardEntry: 몬스터를 쓰러뜨렸을 때 드랍되는 카드 한 장의 정보.
// 아직 등록(소환 해금)되지 않은, 인벤토리에 들고 있는 상태의 데이터
USTRUCT(BlueprintType)
struct FMonsterCardEntry
{
	GENERATED_BODY()

	// MonsterClass: 이 카드가 어떤 몬스터 종족의 카드인지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	// CapturedLevel: 카드가 드랍될 당시 그 몬스터의 레벨 (현재는 표시용 정보일 뿐, 소환 레벨 상한과는 무관)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CapturedLevel = 1;
};

// FRegisteredMonsterEntry: 카드를 소모해 영구로 등록된 몬스터 종족 하나의 도감 항목.
// 같은 종족 카드를 추가로 등록하면 UpgradeCount만 올라간다 (최대 MaxUpgradeCount)
USTRUCT(BlueprintType)
struct FRegisteredMonsterEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseMonster> MonsterClass;

	// UpgradeCount: 최초 등록 이후 추가로 몇 번 더 등록했는지 (0 = 업그레이드 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpgradeCount = 0;

	// Level / CurrentEXP: 소환수로 전투에 참여해 얻은 성장치. 소환 해제/재소환해도 계속 유지됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentEXP = 0;

	// GetEXPToNextLevel: 다음 레벨까지 필요한 경험치 (임시로 Level*100, 플레이어 스탯 성장 공식과 동일)
	int32 GetEXPToNextLevel() const { return Level * 100; }

	// GetUpgradeMultiplier: UpgradeCount(카드 중첩 등록 횟수)로 인한 스탯 배율. 1회당 +15%
	float GetUpgradeMultiplier() const { return 1.f + UpgradeCount * 0.15f; }
};
