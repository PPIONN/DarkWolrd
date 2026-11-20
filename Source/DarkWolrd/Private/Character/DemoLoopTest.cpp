// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/DemoLoopTest.h"

#if !UE_BUILD_SHIPPING

#include "Character/Rabbit.h"
#include "Combat/AbilityComponent.h"
#include "Combat/CombatFaction.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Item/MonsterCardPickup.h"
#include "Inventory/InventoryComponent.h"
#include "Item/ItemPickup.h"
#include "Stats/CharacterStatComponent.h"
#include "UI/InventoryWidget.h"
#include "Monster/BaseMonster.h"
#include "UI/DemoFlowWidget.h"
#include "UI/TutorialGuideWidget.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "UnrealEngine.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float StepInterval = 0.25f;
	constexpr float StartWait = 3.f;
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
	WaitTime = StartWait;
	SetStage(EStage::Title);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FDemoLoopTest::Tick), StepInterval);
	FrameTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(this, &FDemoLoopTest::FrameTick), 0.f);
}


bool FDemoLoopTest::FrameTick(float DeltaTime)
{
	const bool bPlaying = Stage >= EStage::WalkToAim && Stage <= EStage::WalkToExit && Rabbit.IsValid()
		&& Rabbit->GetWorld() && !Rabbit->GetWorld()->IsPaused() && WaitTime <= 0.f;
	if (bPlaying && DeltaTime > 0.f)
	{
		FrameMs.Add(DeltaTime * 1000.f);
		GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
		RenderMs.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
		GpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
	}
	return !bQuitRequested;
}


void FDemoLoopTest::LogPerformance() const
{
	if (FrameMs.Num() == 0) return;

	const auto Stats = [](TArray<float> Values, float& OutAvg, float& OutMedian, float& OutP95, float& OutP99, float& OutMax)
	{
		Values.Sort();
		double Sum = 0.0;
		for (const float Value : Values)
		{
			Sum += Value;
		}
		OutAvg = static_cast<float>(Sum / Values.Num());
		OutMedian = Values[Values.Num() / 2];
		OutP95 = Values[FMath::Min(Values.Num() - 1, FMath::FloorToInt(Values.Num() * 0.95f))];
		OutP99 = Values[FMath::Min(Values.Num() - 1, FMath::FloorToInt(Values.Num() * 0.99f))];
		OutMax = Values.Last();
	};
	float FA, FMed, F95, F99, FMax, GA, GMed, G95, G99, GMax, RA, RMed, R95, R99, RMax, PA, PMed, P95, P99, PMax;
	Stats(FrameMs, FA, FMed, F95, F99, FMax);
	Stats(GameMs, GA, GMed, G95, G99, GMax);
	Stats(RenderMs, RA, RMed, R95, R99, RMax);
	Stats(GpuMs, PA, PMed, P95, P99, PMax);

	double TotalMs = 0.0;
	int32 Over16 = 0;
	for (const float Value : FrameMs)
	{
		TotalMs += Value;
		Over16 += Value > 16.7f ? 1 : 0;
	}
	UE_LOG(LogTemp, Display, TEXT("[LoopTest] perf %dx%d: %d frames in %.1fs, avg %.1f fps (1%% low %.1f fps), frames over 16.7ms %.1f%%"),
		GSystemResolution.ResX, GSystemResolution.ResY, FrameMs.Num(), TotalMs / 1000.0, 1000.f / FA, 1000.f / F99, 100.f * Over16 / FrameMs.Num());
	UE_LOG(LogTemp, Display, TEXT("[LoopTest] perf ms avg/median/p95/p99/max  frame %.2f/%.2f/%.2f/%.2f/%.2f  game %.2f/%.2f/%.2f  render %.2f/%.2f/%.2f  gpu %.2f/%.2f/%.2f/%.2f"),
		FA, FMed, F95, F99, FMax, GA, GMed, G95, RA, RMed, R95, PA, PMed, P95, P99);
}


void FDemoLoopTest::AttachToRabbit(ARabbit* NewRabbit)
{
	Rabbit = NewRabbit;
	LastLocation = NewRabbit->GetActorLocation();
	WaitTime = 1.f;
	if (Stage == EStage::WaitRestart)
	{
		SetStage(EStage::AfterRestart);
	}
}


bool FDemoLoopTest::Tick(float DeltaTime)
{
	Step();
	return !bQuitRequested;
}


void FDemoLoopTest::SetStage(EStage NewStage, const TCHAR* ShotName)
{
	Stage = NewStage;
	StageTime = 0.f;
	StuckTime = 0.f;
	bShotTaken = false;
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
	LogPerformance();
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


FString FDemoLoopTest::DescribeInventory() const
{
	const UInventoryComponent* Inventory = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory) return TEXT("no inventory");

	FString Items;
	for (const FInventorySlot& Slot : Inventory->Slots)
	{
		if (!Slot.IsEmpty())
		{
			Items += FString::Printf(TEXT(" %s x%d,"), *Slot.Item->GetName().Replace(TEXT("DA_Item_"), TEXT("")), Slot.Count);
		}
	}
	FString Equipped;
	for (const UItemDefinition* Item : Inventory->Equipped)
	{
		if (Item)
		{
			Equipped += FString::Printf(TEXT(" %s,"), *Item->GetName().Replace(TEXT("DA_Item_"), TEXT("")));
		}
	}
	return FString::Printf(TEXT("%d slots:%s equipped:%s"), Inventory->GetUsedSlotCount(), *Items, *Equipped);
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
	// 맵을 다시 불러오는 동안은 새 토끼가 붙을 때까지 기다림
	if (!Rabbit.IsValid() || bQuitRequested) return;
	if (WaitTime > 0.f)
	{
		WaitTime -= StepInterval;
		return;
	}

	StageTime += StepInterval;
	TotalTime += StepInterval;
	UMonsterCompanionComponent* Companion = Rabbit->FindComponentByClass<UMonsterCompanionComponent>();
	const UTutorialGuideWidget* Guide = Rabbit->GetGuideWidget();
	const int32 GuideStep = Guide ? static_cast<int32>(Guide->GetCurrentStep()) : -1;
	UDemoFlowWidget* Flow = Rabbit->GetDemoFlowWidget();
	const EDemoScreen Screen = Flow ? Flow->GetScreen() : EDemoScreen::None;

	if (GuideStep != LastGuideStep)
	{
		UE_LOG(LogTemp, Display, TEXT("[LoopTest] guide objective %d at %.0fs"), GuideStep + 1, TotalTime);
		LastGuideStep = GuideStep;
	}

	if (Stage != EStage::Finished && Stage != EStage::FailTest)
	{
		// 쓰러지면 처음 하는 사람처럼 실패 화면에서 '마지막 목표에서 다시'를 고르고 이어감 (봇은 피하지 않으므로 쓰러진 횟수도 난이도 참고값이 됨)
		if (Rabbit->IsDead())
		{
			if (!bWasDead)
			{
				++DeathCount;
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] rabbit died at %.0fs"), TotalTime);
				StopMoving();
			}
			bWasDead = true;
			if (Screen == EDemoScreen::Failed)
			{
				Flow->RetryFromCheckpoint();
			}
			return;
		}
		bWasDead = false;

		if (StageTime > StageTimeLimit)
		{
			Fail(FString::Printf(TEXT("stage %d timed out"), static_cast<int32>(Stage)));
			return;
		}
	}

	// 화면 단계: 처음 한 번은 화면을 찍고, 다음 단계에서 버튼 함수를 부름 (스크린샷이 저장될 틈)
	const auto ShotOnce = [this](const TCHAR* Name)
	{
		if (bShotTaken) return false;
		TakeShot(Name);
		bShotTaken = true;
		return true;
	};

	switch (Stage)
	{
	case EStage::Title:
		if (!Flow)
		{
			Fail(TEXT("no demo flow widget"));
		}
		else if (Screen == EDemoScreen::Title && !ShotOnce(TEXT("LoopTest_0_Title")))
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] title screen ok"));
			Flow->StartGame();
			SetStage(EStage::FailTest);
		}
		break;

	case EStage::FailTest:
		// 실패 화면을 확인하려고 시작하자마자 일부러 한 번 쓰러짐
		if (!bFailTestDied)
		{
			bFailTestDied = true;
			++DeathCount;
			UGameplayStatics::ApplyDamage(Rabbit.Get(), 100000.f, nullptr, nullptr, UDamageType::StaticClass());
		}
		else if (Screen == EDemoScreen::Failed)
		{
			if (!ShotOnce(TEXT("LoopTest_1_Failed")))
			{
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] fail screen ok"));
				Flow->RetryFromCheckpoint();
			}
		}
		else if (Screen == EDemoScreen::None && bShotTaken && !Rabbit->IsDead())
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] retry from checkpoint ok"));
			SetRoute(RouteToAim);
			LastLocation = Rabbit->GetActorLocation();
			SetStage(EStage::WalkToAim, TEXT("LoopTest_1_Start"));
		}
		else if (StageTime > 10.f)
		{
			Fail(TEXT("fail screen did not appear"));
		}
		break;

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
			{
				FString Drops;
				int32 NumDrops = 0;
				for (TActorIterator<AItemPickup> It(Rabbit->GetWorld()); It; ++It)
				{
					++NumDrops;
					Drops += FString::Printf(TEXT(" %s x%d,"), It->Item ? *It->Item->GetName().Replace(TEXT("DA_Item_"), TEXT("")) : TEXT("?"), It->Count);
				}
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] item drops on the ground: %d%s"), NumDrops, *Drops);
			}
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
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] inventory after walking to the card: %s"), *DescribeInventory());
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
			SetStage(EStage::CheckInventory, TEXT("LoopTest_6_Summoned"));
		}
		else
		{
			PressKey(EKeys::Y);
		}
		break;

	case EStage::CheckInventory:
	{
		// I로 인벤토리를 열고 첫 칸을 골라 설명이 보이게 찍은 다음, 장비 빼기·끼기와 물약 사용으로 능력치가 바뀌는지 기록하고 닫음
		UInventoryComponent* Inventory = Rabbit->FindComponentByClass<UInventoryComponent>();
		const UCharacterStatComponent* Stat = Rabbit->FindComponentByClass<UCharacterStatComponent>();
		if (StageTime <= StepInterval)
		{
			// 아이콘이 전부 보이게 아이템을 종류마다 1개씩 넣고 엶
			Rabbit->DebugGiveAllItems();
			PressKey(EKeys::I);
		}
		else if (StageTime >= 1.f && StageTime < 1.f + StepInterval)
		{
			if (UInventoryWidget* InventoryScreen = Cast<UInventoryWidget>(Rabbit->InventoryWidgetNative))
			{
				InventoryScreen->SelectSlot(EInventorySlotKind::Inventory, 0);
			}
		}
		else if (StageTime >= 1.75f && !bShotTaken && Inventory && Stat)
		{
			bShotTaken = true;
			TakeShot(TEXT("LoopTest_6b_Inventory"));
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] inventory open=%d, %s, magic %d, damage x%.2f, max HP %.0f"),
				Rabbit->IsInventoryOpen() ? 1 : 0, *DescribeInventory(), Stat->GetTotalMagicPower(), Stat->GetOutgoingDamageMultiplier(), Stat->GetMaxHP());

			// 무기를 빼면 마력이 기본값으로, 다시 끼면 돌아와야 함
			const int32 MagicWith = Stat->GetTotalMagicPower();
			UItemDefinition* Weapon = Inventory->Equipped.IsValidIndex(0) ? Inventory->Equipped[0] : nullptr;
			if (Weapon && Inventory->Unequip(0))
			{
				const int32 MagicWithout = Stat->GetTotalMagicPower();
				const int32 WandSlot = Inventory->Slots.IndexOfByPredicate([Weapon](const FInventorySlot& Slot) { return Slot.Item == Weapon; });
				const bool bReequipped = Inventory->EquipFromSlot(WandSlot);
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] equipment check: magic %d -> unequip %d -> re-equip %d (%s)"),
					MagicWith, MagicWithout, Stat->GetTotalMagicPower(), bReequipped && MagicWithout < MagicWith && Stat->GetTotalMagicPower() == MagicWith ? TEXT("ok") : TEXT("WRONG"));
			}

			// 물약: HP가 덜 찼으면 써지고 개수가 줄어야 함
			const int32 PotionSlot = Inventory->Slots.IndexOfByPredicate([](const FInventorySlot& Slot) { return !Slot.IsEmpty() && Slot.Item->HealHP > 0.f; });
			if (PotionSlot != INDEX_NONE)
			{
				const int32 Before = Inventory->Slots[PotionSlot].Count;
				const float HPBefore = Stat->CurrentHP;
				FText Message;
				const bool bUsed = Inventory->UseSlot(PotionSlot, Message);
				UE_LOG(LogTemp, Display, TEXT("[LoopTest] potion check: used=%d, count %d -> %d, HP %.0f -> %.0f / %.0f, message '%s'"),
					bUsed ? 1 : 0, Before, Inventory->Slots[PotionSlot].Count, HPBefore, Stat->CurrentHP, Stat->GetMaxHP(), *Message.ToString());
			}
		}
		else if (StageTime >= 2.25f)
		{
			if (Rabbit->IsInventoryOpen())
			{
				PressKey(EKeys::I);
			}
			SetRoute(RouteToBats);
			SetStage(EStage::WalkToBats);
		}
		break;
	}

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
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] exit reached in %.0fs"), TotalTime);
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] inventory at exit: %s"), *DescribeInventory());
			SetStage(EStage::WaitClear);
		}
		else if (FollowRoute())
		{
			Fail(TEXT("reached the exit but the guide did not finish"));
		}
		break;

	case EStage::WaitClear:
		if (Screen == EDemoScreen::Cleared && !ShotOnce(TEXT("LoopTest_8_Clear")))
		{
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] clear screen ok, restarting from the beginning"));
			SetStage(EStage::WaitRestart);
			Flow->RestartFromBeginning();
		}
		else if (StageTime > 10.f)
		{
			Fail(TEXT("clear screen did not appear"));
		}
		break;

	case EStage::WaitRestart:
		// AttachToRabbit에서 다음 단계로 넘어감
		break;

	case EStage::AfterRestart:
		if (Screen == EDemoScreen::Title && !ShotOnce(TEXT("LoopTest_9_Restarted")))
		{
			LogPerformance();
			UE_LOG(LogTemp, Display, TEXT("[LoopTest] PASS in %.0fs, rabbit died %d times (1 on purpose)"), TotalTime, DeathCount);
			SetStage(EStage::Finished);
		}
		else if (StageTime > 10.f)
		{
			Fail(TEXT("title screen did not appear after restart"));
		}
		break;

	case EStage::Finished:
		// 마지막 화면이 저장될 시간을 주고 종료 버튼 함수로 끝냄
		if (StageTime >= 1.f)
		{
			bQuitRequested = true;
			if (Flow)
			{
				Flow->QuitDemo();
			}
			else
			{
				FPlatformMisc::RequestExit(false, TEXT("DemoLoopTest"));
			}
		}
		break;
	}
}

#endif
