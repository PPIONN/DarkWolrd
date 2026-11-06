// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/DemoLoopTest.h"

#if !UE_BUILD_SHIPPING

#include "Character/Rabbit.h"
#include "Combat/AbilityComponent.h"
#include "Combat/CombatFaction.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Item/MonsterCardPickup.h"
#include "Monster/BaseMonster.h"
#include "UI/TutorialGuideWidget.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "InputKeyEventArgs.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr float StepInterval = 0.25f;
	constexpr float ArriveRadius = 300.f;
	constexpr float CardArriveRadius = 40.f;
	constexpr float AttackRange = 900.f;
	constexpr float StuckSpeedLimit = 30.f;
	constexpr float StuckJumpTime = 2.f;
	constexpr float StuckFailTime = 20.f;
	constexpr float StageTimeLimit = 90.f;

	// 튜토리얼 맵의 길 (맵을 만들 때 쓴 걷기 통로 좌표)
	const TArray<FVector> RouteToAim = { FVector(-8500, -6000, 0), FVector(-5500, -5200, 0), FVector(-4500, -2500, 0), FVector(-1500, -1500, 0) };
	const TArray<FVector> RouteToHunt = { FVector(1200, -2500, 0), FVector(3000, 0, 0) };
	const TArray<FVector> RouteToBats = { FVector(1800, 3500, 0), FVector(4500, 5800, 0) };
	const TArray<FVector> RouteToExit = { FVector(8000, 5500, 0), FVector(9800, 9300, 0) };
}

FDemoLoopTest::FDemoLoopTest(ARabbit* InRabbit)
	: Rabbit(InRabbit)
{
}


void FDemoLoopTest::Start()
{
	UE_LOG(LogTemp, Display, TEXT("[LoopTest] START"));
	SetRoute(RouteToAim);
	LastLocation = Rabbit->GetActorLocation();
	TakeShot(TEXT("LoopTest_1_Start"));
	Rabbit->GetWorldTimerManager().SetTimer(TimerHandle,
		FTimerDelegate::CreateSP(this, &FDemoLoopTest::Step), StepInterval, true, 3.f);
}


void FDemoLoopTest::SetStage(EStage NewStage, const TCHAR* ShotName)
{
	Stage = NewStage;
	StageTime = 0.f;
	StuckTime = 0.f;
	if (ShotName)
	{
		TakeShot(ShotName);
	}
}


void FDemoLoopTest::SetRoute(TArray<FVector> NewRoute)
{
	Route = MoveTemp(NewRoute);
	RouteIndex = 0;
}


void FDemoLoopTest::Fail(const FString& Reason)
{
	UE_LOG(LogTemp, Display, TEXT("[LoopTest] FAIL at %.0fs, %s: %s"), TotalTime, *Rabbit->GetActorLocation().ToCompactString(), *Reason);
	StopMoving();
	SetStage(EStage::Finished, TEXT("LoopTest_Fail"));
}


void FDemoLoopTest::PressKey(const FKey& Key) const
{
	HoldKey(Key);
	ReleaseKey(Key);
}


void FDemoLoopTest::HoldKey(const FKey& Key) const
{
	if (APlayerController* PC = Cast<APlayerController>(Rabbit->GetController()))
	{
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
	}
}


void FDemoLoopTest::ReleaseKey(const FKey& Key) const
{
	if (APlayerController* PC = Cast<APlayerController>(Rabbit->GetController()))
	{
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
	}
}


void FDemoLoopTest::StopMoving() const
{
	ReleaseKey(EKeys::W);
	ReleaseKey(EKeys::LeftShift);
}


void FDemoLoopTest::TakeShot(const FString& Name) const
{
	FScreenshotRequest::RequestScreenshot(Name + TEXT(".png"), true, false);
}


void FDemoLoopTest::LookAt(const FVector& Location) const
{
	if (AController* Controller = Rabbit->GetController())
	{
		Controller->SetControlRotation((Location - Rabbit->GetActorLocation()).Rotation());
	}
}


void FDemoLoopTest::WalkTowards(const FVector& Location)
{
	const FVector Here = Rabbit->GetActorLocation();
	StuckTime = FVector::Dist2D(Here, LastLocation) < StuckSpeedLimit * StepInterval ? StuckTime + StepInterval : 0.f;
	LastLocation = Here;

	if (AController* Controller = Rabbit->GetController())
	{
		Controller->SetControlRotation(FRotator(-5.f, (Location - Here).Rotation().Yaw, 0.f));
	}
	HoldKey(EKeys::LeftShift);
	HoldKey(EKeys::W);

	if (StuckTime >= StuckFailTime)
	{
		Fail(TEXT("stuck while walking"));
	}
	else if (StuckTime >= StuckJumpTime)
	{
		PressKey(EKeys::SpaceBar);
	}
}


bool FDemoLoopTest::FollowRoute()
{
	if (FVector::Dist2D(Rabbit->GetActorLocation(), Route[RouteIndex]) < ArriveRadius && ++RouteIndex >= Route.Num())
	{
		StopMoving();
		return true;
	}
	WalkTowards(Route[RouteIndex]);
	return false;
}


ABaseMonster* FDemoLoopTest::FindWildMonster() const
{
	ABaseMonster* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<ABaseMonster> It(Rabbit->GetWorld()); It; ++It)
	{
		if (It->IsAlive() && !It->bIsPlayerSummon && CombatFaction::AreHostile(Rabbit.Get(), *It))
		{
			const float DistSq = FVector::DistSquared(It->GetActorLocation(), Rabbit->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = *It;
			}
		}
	}
	return Nearest;
}


void FDemoLoopTest::FightNearestMonster()
{
	Target = FindWildMonster();
	if (!Target.IsValid()) return;

	if (FVector::Dist2D(Target->GetActorLocation(), Rabbit->GetActorLocation()) > AttackRange)
	{
		WalkTowards(Target->GetActorLocation());
		return;
	}

	StopMoving();
	LookAt(Target->GetActorLocation());
	const UAbilityComponent* Ability = Rabbit->FindComponentByClass<UAbilityComponent>();
	if (Ability && !Ability->IsLockedOn())
	{
		PressKey(EKeys::Q);
	}
	PressKey(EKeys::LeftMouseButton);
}


void FDemoLoopTest::Step()
{
	if (!Rabbit.IsValid()) return;

	StageTime += StepInterval;
	TotalTime += StepInterval;
	UMonsterCompanionComponent* Companion = Rabbit->FindComponentByClass<UMonsterCompanionComponent>();
	const UTutorialGuideWidget* Guide = Rabbit->GetGuideWidget();
	const int32 GuideStep = Guide ? static_cast<int32>(Guide->GetCurrentStep()) : -1;

	if (GuideStep != LastGuideStep)
	{
		UE_LOG(LogTemp, Display, TEXT("[LoopTest] guide objective %d at %.0fs"), GuideStep + 1, TotalTime);
		LastGuideStep = GuideStep;
	}

	if (Stage != EStage::Finished)
	{
		// 쓰러지면 처음 하는 사람처럼 부활을 기다렸다가 이어감 (봇은 피하지 않으므로 몇 번 쓰러졌는지도 난이도 참고값이 됨)
		if (Rabbit->IsDead())
		{
			if (!bWasDead)
			{
				++DeathCount;
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] rabbit died at %.0fs"), TotalTime);
				StopMoving();
			}
			bWasDead = true;
			return;
		}
		bWasDead = false;

		if (StageTime > StageTimeLimit)
		{
			Fail(FString::Printf(TEXT("stage %d timed out"), static_cast<int32>(Stage)));
			return;
		}
	}

	switch (Stage)
	{
	case EStage::WalkToAim:
		if (FollowRoute())
		{
			StartAttackCount = Rabbit->GetAttackCount();
			SetStage(EStage::ShootTwice, TEXT("LoopTest_2_AimZone"));
		}
		break;

	case EStage::ShootTwice:
		PressKey(EKeys::LeftMouseButton);
		if (GuideStep > static_cast<int32>(ETutorialStep::FirstShot))
		{
			SetRoute(RouteToHunt);
			SetStage(EStage::WalkToHunt);
		}
		break;

	case EStage::WalkToHunt:
		if (FollowRoute())
		{
			// 내비 메시가 없으면 지상 몬스터와 소환수의 길찾기가 안 되므로 여기서 함께 확인
			const UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(Rabbit->GetWorld());
			FNavLocation NavLocation;
			if (!NavSystem || !NavSystem->ProjectPointToNavigation(Rabbit->GetActorLocation(), NavLocation, FVector(100.f, 100.f, 300.f)))
			{
				Fail(TEXT("no navmesh at the hunt zone"));
				break;
			}
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] navmesh ok at hunt zone"));
			StartKillCount = Companion->WildKillCount;
			SetStage(EStage::KillFirst, TEXT("LoopTest_3_HuntZone"));
		}
		break;

	case EStage::KillFirst:
		if (Companion->WildKillCount > StartKillCount)
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] first monster killed"));
			StopMoving();
			SetStage(EStage::PickUpCard, TEXT("LoopTest_4_CardDropped"));
		}
		else
		{
			FightNearestMonster();
		}
		break;

	case EStage::PickUpCard:
		if (Companion->RegisteredMonsters.Num() > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] card registered: %d species"), Companion->RegisteredMonsters.Num());
			StopMoving();
			SetStage(EStage::Summon, TEXT("LoopTest_5_CardRegistered"));
		}
		else
		{
			const AMonsterCardPickup* Card = nullptr;
			for (TActorIterator<AMonsterCardPickup> It(Rabbit->GetWorld()); It; ++It)
			{
				Card = *It;
				break;
			}
			if (Card && FVector::Dist2D(Card->GetActorLocation(), Rabbit->GetActorLocation()) > CardArriveRadius)
			{
				WalkTowards(Card->GetActorLocation());
			}
		}
		break;

	case EStage::Summon:
		if (Companion->AreSummonsActive())
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] summoned: %d active"), Companion->ActiveSummons.Num());
			SetRoute(RouteToBats);
			SetStage(EStage::WalkToBats, TEXT("LoopTest_6_Summoned"));
		}
		else
		{
			PressKey(EKeys::Y);
		}
		break;

	case EStage::WalkToBats:
		if (FollowRoute())
		{
			StartKillCount = Companion->WildKillCount;
			Target = FindWildMonster();
			WatchedHP = Target.IsValid() ? Target->CurrentHP : 0.f;
			SetStage(EStage::WatchSummons);
		}
		break;

	case EStage::WatchSummons:
		// 여기서는 플레이어가 공격하지 않으므로, 몬스터의 HP가 줄면 소환수가 싸운 것
		if (Companion->WildKillCount > StartKillCount || (Target.IsValid() && Target->CurrentHP < WatchedHP))
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] summons dealt damage after %.1fs"), StageTime);
			SetStage(EStage::HuntTogether, TEXT("LoopTest_7_FightTogether"));
		}
		else if (!Target.IsValid() || !Target->IsAlive())
		{
			Target = FindWildMonster();
			WatchedHP = Target.IsValid() ? Target->CurrentHP : 0.f;
		}
		else if (FVector::Dist2D(Target->GetActorLocation(), Rabbit->GetActorLocation()) > AttackRange)
		{
			WalkTowards(Target->GetActorLocation());
		}
		else
		{
			StopMoving();
		}
		if (!Companion->AreSummonsActive())
		{
			PressKey(EKeys::Y);
		}
		break;

	case EStage::HuntTogether:
		if (GuideStep > static_cast<int32>(ETutorialStep::HuntTogether))
		{
			StopMoving();
			SetRoute(RouteToExit);
			SetStage(EStage::WalkToExit);
		}
		else
		{
			// 소환수가 쓰러졌으면 안내대로 다시 부름
			if (!Companion->AreSummonsActive())
			{
				PressKey(EKeys::Y);
			}
			FightNearestMonster();
		}
		break;

	case EStage::WalkToExit:
		if (GuideStep == static_cast<int32>(ETutorialStep::Done))
		{
			StopMoving();
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] PASS in %.0fs, rabbit died %d times"), TotalTime, DeathCount);
			SetStage(EStage::Finished, TEXT("LoopTest_8_Done"));
		}
		else if (FollowRoute())
		{
			Fail(TEXT("reached the exit but the guide did not finish"));
		}
		break;

	case EStage::Finished:
		// 마지막 화면이 저장될 시간을 주고 Esc로 끝냄 (종료 키도 같이 확인됨). 쓰러진 동안은 입력이 막혀 있어 부활을 기다림
		if (StageTime >= 1.f && !Rabbit->IsDead())
		{
			PressKey(EKeys::Escape);
		}
		break;
	}
}

#endif
