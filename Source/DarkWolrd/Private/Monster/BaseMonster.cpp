// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/BaseMonster.h"
#include "Item/MonsterCardPickup.h"
#include "Summon/SummonAIComponent.h"
#include "AI/WildMonsterAIComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Stats/CharacterStatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"
#include "Components/WidgetComponent.h"
#include "UI/MonsterHPBarWidget.h"
#include "UI/DamageNumberActor.h"
#include "Character/GameFeelComponent.h"
#include "Character/Rabbit.h"

// 생성자: Tick 활성화 및 기본 설정
ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	SummonAIComponent = CreateDefaultSubobject<USummonAIComponent>(TEXT("SummonAIComponent"));
	WildAIComponent = CreateDefaultSubobject<UWildMonsterAIComponent>(TEXT("WildAIComponent"));
	GameFeelComponent = CreateDefaultSubobject<UGameFeelComponent>(TEXT("GameFeelComponent"));

	// 레벨에 배치한 몬스터뿐 아니라 코드로 스폰한 몬스터(필드보스, 던전 웨이브, 소환수)도 AI 컨트롤러가 붙도록 함
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 머리 위 HP바: 화면 고정 크기로 그림. 높이는 BeginPlay에서 캡슐 크기에 맞춰 정함
	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(GetRootComponent());
	HPBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarComponent->SetDrawSize(FVector2D(90.f, 9.f));
	HPBarComponent->SetPivot(FVector2D(0.5f, 0.5f));
	HPBarComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPBarComponent->SetWidgetClass(UMonsterHPBarWidget::StaticClass());
	HPBarComponent->SetVisibility(false);
}

// 게임 시작 시 한 번 호출: 현재 체력을 최대 체력으로 초기화
void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 현재 체력을 최대 체력으로 설정
	CurrentHP = GetMaxHP();

	// HP바: 캡슐 꼭대기 위에 띄우고, 내 소환수는 초록·항상 표시, 적은 빨강·맞았을 때만 표시
	if (HPBarComponent)
	{
		HPBarComponent->SetRelativeLocation(FVector(0.f, 0.f, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + HPBarHeightOffset));
		if (UMonsterHPBarWidget* HPBarWidget = Cast<UMonsterHPBarWidget>(HPBarComponent->GetUserWidgetObject()))
		{
			HPBarWidget->SetMonster(this, bIsPlayerSummon ? SummonHPBarColor : EnemyHPBarColor);
		}
		SetHPBarVisible(bIsPlayerSummon || bAlwaysShowHPBar);
	}
}


void ABaseMonster::SetHPBarVisible(bool bVisible)
{
	if (HPBarComponent)
	{
		HPBarComponent->SetVisibility(bVisible);
	}
}


void ABaseMonster::ShowHPBarAfterDamage()
{
	SetHPBarVisible(true);
	if (bIsPlayerSummon || bAlwaysShowHPBar) return;

	GetWorldTimerManager().SetTimer(HPBarHideTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetHPBarVisible(false);
	}), HPBarHideDelay, false);
}


void ABaseMonster::SpawnDamageNumber(float Damage, const AActor* DamageCauser) const
{
	// 투사체면 쏜 쪽을 공격자로 봄
	const AActor* Attacker = (DamageCauser && !DamageCauser->IsA<APawn>() && DamageCauser->GetInstigator()) ? DamageCauser->GetInstigator() : DamageCauser;

	FLinearColor Color = FLinearColor(0.95f, 0.95f, 0.95f);
	int32 FontSize = 20;
	if (bIsPlayerSummon)
	{
		Color = FLinearColor(1.f, 0.3f, 0.25f);
		FontSize = 18;
	}
	else if (Attacker && Attacker->FindComponentByClass<UCharacterStatComponent>())
	{
		Color = FLinearColor(1.f, 0.85f, 0.25f);
		FontSize = 26;
	}

	const FVector HeadLocation = GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	ADamageNumberActor::SpawnDamageNumber(GetWorld(), HeadLocation, Damage, Color, FontSize);
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
	if (!IsAlive()) return 0.f;

	// 몬스터대전 상대처럼 플레이어 캐릭터를 무시하는 몬스터는 토끼가 직접(마법 구체 등) 주는 피해를 받지 않음
	if (bIgnoresPlayerCharacter && DamageCauser)
	{
		const AActor* Attacker = (!DamageCauser->IsA<APawn>() && DamageCauser->GetInstigator()) ? DamageCauser->GetInstigator() : DamageCauser;
		if (Attacker->FindComponentByClass<UCharacterStatComponent>()) return 0.f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 저주 상태면 실제로 받는 피해량 자체를 증폭시킴 (모든 공격자·모든 대미지 소스에 공통 적용)
	ActualDamage *= CurseDamageAmplifyMultiplier;

	// 처치 보상 받을 플레이어 기록: 투사체면 쏜 쪽, 소환수면 그 주인, 스탯을 가진 캐릭터(플레이어)면 본인
	if (AActor* Attacker = DamageCauser)
	{
		if (!Attacker->IsA<APawn>() && Attacker->GetInstigator())
		{
			Attacker = Attacker->GetInstigator();
		}
		if (const ABaseMonster* AttackerMonster = Cast<ABaseMonster>(Attacker))
		{
			if (AttackerMonster->bIsPlayerSummon && AttackerMonster->SummonAIComponent && AttackerMonster->SummonAIComponent->GetOwnerPlayer())
			{
				LastRewardPlayer = AttackerMonster->SummonAIComponent->GetOwnerPlayer();
			}
		}
		else if (Attacker->FindComponentByClass<UCharacterStatComponent>())
		{
			LastRewardPlayer = Attacker;

			// 플레이어가 직접 때린 적이면 소환수들이 함께 공격하도록 알림
			if (UMonsterCompanionComponent* Companion = Attacker->FindComponentByClass<UMonsterCompanionComponent>())
			{
				Companion->NotifyPlayerHitEnemy(this);
			}
		}
	}

	CurrentHP -= ActualDamage;

	if (ActualDamage > 0.f)
	{
		ShowHPBarAfterDamage();
		if (bShowDamageNumbers)
		{
			SpawnDamageNumber(ActualDamage, DamageCauser);
		}

		// 타격감: 맞은 몬스터가 납작해지며 잠깐 멈춤. 플레이어가 때렸으면 더 길게 멈추고 화면도 흔들림.
		// 근접 공격(투사체가 아님)이면 때린 쪽도 같이 멈춰서 "걸리는" 손맛을 줌
		const bool bKilled = CurrentHP <= 0.f;
		AActor* Attacker = (DamageCauser && !DamageCauser->IsA<APawn>() && DamageCauser->GetInstigator()) ? DamageCauser->GetInstigator() : DamageCauser;
		ARabbit* PlayerAttacker = Cast<ARabbit>(Attacker);
		if (GameFeelComponent)
		{
			GameFeelComponent->PlayHitReaction(PlayerAttacker != nullptr, bKilled);
		}
		if (PlayerAttacker)
		{
			PlayerAttacker->AddCameraTrauma(bKilled ? PlayerAttacker->KillCameraTrauma : PlayerAttacker->HitCameraTrauma);
		}
		if (DamageCauser && DamageCauser->IsA<APawn>())
		{
			if (UGameFeelComponent* AttackerFeel = DamageCauser->FindComponentByClass<UGameFeelComponent>())
			{
				AttackerFeel->HitStop(AttackerFeel->HitStopLight);
			}
		}
	}

	// 공격받았음을 AI에 통보: 소환수는 자기방어 우선순위, 야생 몬스터는 반격 대상 지정
	if (DamageCauser)
	{
		if (bIsPlayerSummon && SummonAIComponent)
		{
			SummonAIComponent->NotifyAttacked(DamageCauser);
		}
		else if (WildAIComponent)
		{
			WildAIComponent->NotifyAttacked(DamageCauser);
		}
	}

	if (CurrentHP <= 0.f)
	{
		CurrentHP = 0.f;
		Die();
	}
	else if (HitMontage && !GetCurrentMontage())
	{
		// 공격 몽타주를 끊으면 타격 타이밍이 어긋나 보이므로, 다른 몽타주가 없을 때만 움찔함
		PlayAnimMontage(HitMontage);
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


// 사망 처리: 카드 드랍(야생) 또는 소환주에게 통보(소환수) 후, AI·이동·충돌을 끄고
// DeathAnimation이 있으면 재생 → 마지막 포즈로 CorpseLingerTime만큼 남았다가 사라짐. 없으면 즉시 제거
void ABaseMonster::Die()
{
	// 소환수는 자기 자신의 카드를 드랍하지 않음 (야생 몬스터 사망 시에만 해당).
	// 대신 소환주에게 알려서 소환 목록과 파트너 패시브에서 빠지게 함
	if (!bIsPlayerSummon)
	{
		TryDropMonsterCard();
		GrantKillRewards();
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

	if (WildAIComponent)
	{
		WildAIComponent->StopWildAI();
	}

	OnMonsterDied.Broadcast(this);

	// 시체가 더 이상 움직이거나 길을 막지 않도록 컨트롤러를 떼고 이동·충돌을 끔
	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 한 방에 쓰러져도 HP바가 보이도록, 0이 된 바(잔상이 줄어드는 모습)를 HPBarDeathLingerTime 동안 보여준 뒤 숨김
	SetHPBarVisible(true);
	GetWorldTimerManager().SetTimer(HPBarHideTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetHPBarVisible(false);
	}), HPBarDeathLingerTime, false);

	// 사망 애니메이션이 없으면 몸은 바로 감추고, HP바가 사라질 때까지만 액터를 남겨둠
	if (!DeathAnimation)
	{
		GetMesh()->SetVisibility(false, true);
		SetLifeSpan(HPBarDeathLingerTime + 0.1f);
		return;
	}

	// 애님 BP 대신 이 시퀀스 하나만 반복 없이 재생 → 끝나면 마지막 포즈에서 멈춰 있음
	GetMesh()->PlayAnimation(DeathAnimation, /*bLooping=*/false);
	SetLifeSpan(FMath::Max(DeathAnimation->GetPlayLength() + CorpseLingerTime, HPBarDeathLingerTime + 0.1f));
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

// 공격력: 1레벨 기준값 + 레벨업 보너스. 소환수는 카드 중첩 등록 배율, 보스는 페이즈 강화 배율이 추가로 곱해짐
float ABaseMonster::GetAttackPower() const
{
	return (BaseAttackPower + (Level - 1) * AttackPowerPerLevel) * SummonPowerMultiplier * AttackPowerBuffMultiplier;
}


// 처치 보상: 마지막으로 때린 플레이어(또는 소환수의 주인)에게 경험치와 돈 지급
void ABaseMonster::GrantKillRewards()
{
	AActor* RewardPlayer = LastRewardPlayer.Get();
	UCharacterStatComponent* PlayerStat = RewardPlayer ? RewardPlayer->FindComponentByClass<UCharacterStatComponent>() : nullptr;
	if (!PlayerStat) return;

	PlayerStat->AddExperience(FMath::RoundToInt(RewardEXP));
	PlayerStat->AddMoney(GetMoneyDropAmount());
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

	UClass* PickupClass = CardPickupClass ? CardPickupClass.Get() : AMonsterCardPickup::StaticClass();
	AMonsterCardPickup* CardPickup = GetWorld()->SpawnActor<AMonsterCardPickup>(
		PickupClass, GetActorLocation(), GetActorRotation());

	if (CardPickup)
	{
		CardPickup->CardData.MonsterClass = GetClass();
		CardPickup->CardData.CapturedLevel = Level;
	}
}


// 표시 이름: MonsterName이 비어있으면 클래스 이름에서 BP_ 접두사와 _C 접미사를 떼서 사용
FText ABaseMonster::GetMonsterDisplayName() const
{
	if (!MonsterName.IsEmpty()) return MonsterName;

	FString ClassName = GetClass()->GetName();
	ClassName.RemoveFromStart(TEXT("BP_"));
	ClassName.RemoveFromEnd(TEXT("_C"));
	return FText::FromString(ClassName);
}


// 소환 등장 연출: 메시 크기를 거의 0에서 시작해 살짝 넘쳤다가(ease-out-back) 원래 크기로 맞춤. 발밑 기준으로 커지므로 땅에서 솟아나는 느낌
void ABaseMonster::PlaySummonAppear(float Duration)
{
	// 크기는 GameFeelComponent를 거쳐서 바꿔야 피격 찌그러짐 등 다른 연출과 겹쳐도 원래 크기가 틀어지지 않음
	if (!GameFeelComponent || Duration <= 0.f) return;

	const float StartTime = GetWorld()->GetTimeSeconds();
	GameFeelComponent->SetAppearScale(0.05f);

	GetWorldTimerManager().SetTimer(SummonAppearTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, StartTime, Duration]()
	{
		const float Alpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - StartTime) / Duration, 0.f, 1.f);
		if (Alpha >= 1.f)
		{
			GameFeelComponent->SetAppearScale(1.f);
			GetWorldTimerManager().ClearTimer(SummonAppearTimerHandle);
			return;
		}

		const float Overshoot = 1.70158f;
		const float T = Alpha - 1.f;
		const float Eased = 1.f + (Overshoot + 1.f) * T * T * T + Overshoot * T * T;
		GameFeelComponent->SetAppearScale(FMath::Max(Eased, 0.05f));
	}), 0.016f, true);
}
