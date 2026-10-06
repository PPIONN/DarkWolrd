// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/BaseMonster.h"
#include "Item/MonsterCardPickup.h"
#include "Summon/SummonAIComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

// 생성자: Tick 활성화 및 기본 설정
ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	SummonAIComponent = CreateDefaultSubobject<USummonAIComponent>(TEXT("SummonAIComponent"));
}

// 게임 시작 시 한 번 호출: 현재 체력을 최대 체력으로 초기화
void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 현재 체력을 최대 체력으로 설정
	CurrentHP = GetMaxHP();
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

	// 저주 상태면 실제로 받는 피해량 자체를 증폭시킴 (모든 공격자·모든 대미지 소스에 공통 적용)
	ActualDamage *= CurseDamageAmplifyMultiplier;

	CurrentHP -= ActualDamage;

	// 소환수라면 공격받았음을 자기방어 AI에 통보 (야생 몬스터는 컴포넌트가 비활성 상태라 아무 일도 안 함)
	if (bIsPlayerSummon && SummonAIComponent && DamageCauser)
	{
		SummonAIComponent->NotifyAttacked(DamageCauser);
	}

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		Die();
	}

	return ActualDamage;
}

// 저주 적용(또는 갱신): 배율은 최신 값으로 덮어쓰고(중첩 없음) 지속시간 타이머를 새로고침.
// 저주가 처음 걸리는 순간에만 VFX를 새로 스폰하고, 이미 걸려있는 중이면 기존 VFX를 그대로 유지한다
void ABaseMonster::ApplyCurse(float AmplifyPercent, float Duration)
{
	if (!IsCursed() && CurseVFX)
	{
		CurseVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CurseVFX, GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, /*bAutoDestroy=*/false);
	}

	CurseDamageAmplifyMultiplier = 1.f + (AmplifyPercent / 100.f);

	// 이미 걸려있던 중이어도 SetTimer가 같은 핸들을 덮어써서 지속시간이 그대로 새로고침됨
	GetWorld()->GetTimerManager().SetTimer(CurseTimerHandle, this, &ABaseMonster::ClearCurse, Duration, false);
}


// 저주 해제: 배율을 원래대로 되돌리고 VFX를 제거
void ABaseMonster::ClearCurse()
{
	CurseDamageAmplifyMultiplier = 1.f;

	if (CurseVFXComponent)
	{
		CurseVFXComponent->DestroyComponent();
		CurseVFXComponent = nullptr;
	}
}


// 사망 처리: 기본 구현은 카드 드랍 판정 후 액터를 세상에서 제거. 자식 클래스에서 재정의하여 사망 애니메이션 등 추가 가능
void ABaseMonster::Die()
{
	// 소환수는 자기 자신의 카드를 드랍하지 않음 (야생 몬스터 사망 시에만 해당).
	// 대신 소환주에게 알려서 소환 목록과 파트너 패시브에서 빠지게 함
	if (!bIsPlayerSummon)
	{
		TryDropMonsterCard();
	}
	else if (SummonAIComponent)
	{
		if (AActor* OwnerPlayer = SummonAIComponent->GetOwnerPlayer())
		{
			if (UMonsterCompanionComponent* Companion = OwnerPlayer->FindComponentByClass<UMonsterCompanionComponent>())
			{
				Companion->HandleSummonDied(this);
			}
		}
		SummonAIComponent->DeactivateSummonAI();
	}

	// TODO: 사망 애니메이션, EXP 지급 등 자식 클래스에서 구현
	Destroy();
}

// 생존 여부 확인: 현재 체력이 0보다 크면 살아있음
bool ABaseMonster::IsAlive() const
{
	return CurrentHP > 0.f;
}

// 최대 HP: 1레벨 기준값 + 레벨업 보너스. 소환수는 카드 중첩 등록에 따른 배율이 추가로 곱해짐
float ABaseMonster::GetMaxHP() const
{
	return (BaseMaxHP + (Level - 1) * HPPerLevel) * SummonPowerMultiplier;
}

// 공격력: 1레벨 기준값 + 레벨업 보너스. 소환수는 카드 중첩 등록에 따른 배율이 추가로 곱해짐
float ABaseMonster::GetAttackPower() const
{
	return (BaseAttackPower + (Level - 1) * AttackPowerPerLevel) * SummonPowerMultiplier;
}

// 플레이어에게 주는 피해량 계산: (플레이어 최대 HP × 기본 비율) + 몬스터 공격력 - (플레이어 방어력 × 0.5), 최소 1
float ABaseMonster::CalculatePlayerDamage(float PlayerMaxHP, float PlayerDefense) const
{
	const float RawDamage = (PlayerMaxHP * BaseDamagePercentOfPlayerMaxHP) + GetAttackPower() - (PlayerDefense * 0.5f);
	return FMath::Max(RawDamage, 1.f);
}

// 드랍 돈 계산: 10 + (레벨×5) + 0~20 랜덤
int32 ABaseMonster::GetMoneyDropAmount() const
{
	return 10 + (Level * 5) + FMath::RandRange(0, 20);
}

// 카드 드랍 판정: CardDropChance 확률로 이 몬스터 종족의 카드를 죽은 위치에 스폰
void ABaseMonster::TryDropMonsterCard()
{
	if (FMath::FRand() > CardDropChance) return;

	AMonsterCardPickup* CardPickup = GetWorld()->SpawnActor<AMonsterCardPickup>(
		AMonsterCardPickup::StaticClass(), GetActorLocation(), GetActorRotation());

	if (CardPickup)
	{
		CardPickup->CardData.MonsterClass = GetClass();
		CardPickup->CardData.CapturedLevel = Level;
	}
}
