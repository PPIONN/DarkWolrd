// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/BaseMonster.h"

// 생성자: Tick 활성화 및 기본 설정
ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;
}

// 게임 시작 시 한 번 호출: 현재 체력을 최대 체력으로 초기화
void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 현재 체력을 최대 체력으로 설정
	CurrentHP = MaxHP;
}

// 매 프레임 호출: 현재는 별도 로직 없음. 자식 클래스에서 AI 등 추가 가능
void ABaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 데미지 처리: 받은 데미지만큼 현재 체력을 깎고, 0 이하가 되면 Die() 호출
float ABaseMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!IsAlive()) return 0.f;

	CurrentHP -= ActualDamage;

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		Die();
	}

	return ActualDamage;
}

// 사망 처리: 기본 구현은 액터를 세상에서 제거. 자식 클래스에서 재정의하여 사망 애니메이션 등 추가 가능
void ABaseMonster::Die()
{
	// TODO: 사망 애니메이션, EXP 지급 등 자식 클래스에서 구현
	Destroy();
}

// 생존 여부 확인: 현재 체력이 0보다 크면 살아있음
bool ABaseMonster::IsAlive() const
{
	return CurrentHP > 0.f;
}
