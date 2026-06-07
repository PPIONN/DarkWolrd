// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// ACharacter: 이동, 점프, 충돌 등 기본 기능이 내장된 캐릭터 기본 클래스
#include "GameFramework/Character.h"
#include "BaseMonster.generated.h"

// ABaseMonster: 모든 몬스터의 부모(베이스) 클래스
// 새 몬스터를 만들 때 이 클래스를 상속받아 사용하면 됨
// 'A'는 Actor(게임 세상에 존재하는 물체)를 의미
UCLASS()
class DARKWOLRD_API ABaseMonster : public ACharacter
{
	// GENERATED_BODY: 언리얼 엔진이 자동으로 필요한 코드를 생성해주는 매크로
	GENERATED_BODY()

public:
	// 생성자: 몬스터가 처음 만들어질 때 기본값을 설정
	ABaseMonster();

protected:
	// BeginPlay: 몬스터가 게임 세상에 등장할 때 한 번 호출되는 함수
	virtual void BeginPlay() override;

public:
	// Tick: 매 프레임마다 자동으로 호출되는 함수
	virtual void Tick(float DeltaTime) override;

	// ─── 몬스터 기본 스탯 ───────────────────────────────────────

	// EditAnywhere: 에디터와 블루프린트에서 값을 직접 수정 가능
	// BlueprintReadWrite: 블루프린트에서 이 변수를 읽고 쓸 수 있음
	// Category: 에디터에서 표시될 분류 탭 이름

	// Level: 몬스터의 레벨. 레벨에 따라 스탯이 달라지도록 확장 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	int32 Level = 1;

	// MaxHP: 몬스터의 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float MaxHP = 100.f;

	// CurrentHP: 몬스터의 현재 체력. 0이 되면 사망 처리
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Stats")
	float CurrentHP;

	// AttackPower: 몬스터가 플레이어에게 주는 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float AttackPower = 10.f;

	// RewardEXP: 몬스터를 처치했을 때 플레이어가 얻는 경험치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float RewardEXP = 20.f;

	// ─── 몬스터 기본 함수 ───────────────────────────────────────

	// TakeDamage: 몬스터가 데미지를 받을 때 호출되는 함수. 반환값은 실제로 받은 데미지
	// 언리얼 엔진 기본 데미지 시스템을 오버라이드(재정의)함
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// Die: 몬스터가 사망했을 때 호출되는 함수. 자식 클래스에서 재정의하여 사망 연출 추가 가능
	virtual void Die();

	// IsAlive: 몬스터가 살아있는지 확인하는 함수. true면 살아있음
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	bool IsAlive() const;
};
