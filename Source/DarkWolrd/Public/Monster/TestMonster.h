// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Monster/BaseMonster.h"
#include "TestMonster.generated.h"

// ATestMonster: 전투/스탯 시스템을 눈으로 확인해보기 위한 테스트용 몬스터.
// BaseMonster의 스탯(레벨1 기준 HP/공격력 등)을 그대로 쓰고, 레벨 에디터에 바로 배치해서
// 볼 수 있도록 기존 매니퀸(SKM_Manny) 메시만 붙여둔 것 - 별도 애니메이션은 없어 T포즈로 보임
UCLASS()
class DARKWOLRD_API ATestMonster : public ABaseMonster
{
	GENERATED_BODY()

public:
	ATestMonster();
};
