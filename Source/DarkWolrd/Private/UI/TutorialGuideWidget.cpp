// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TutorialGuideWidget.h"
#include "Character/Rabbit.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Item/MonsterCardPickup.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

// 튜토리얼 맵의 구역 중심 (월드 좌표 cm). 맵 구역을 옮기면 같이 고쳐야 함
namespace TutorialZones
{
	const FVector Jump(-5500.f, -5200.f, 200.f);
	const FVector Aim(-1500.f, -1500.f, 0.f);
	const FVector FirstHunt(3000.f, 0.f, -250.f);
	const FVector Summon(4500.f, 5800.f, -150.f);
	const FVector Exit(9800.f, 9300.f, 500.f);

	constexpr float JumpArriveRadius = 900.f;
	constexpr float AimArriveRadius = 1500.f;
	constexpr float ExitArriveRadius = 900.f;

	// MarkerHideDistance: 목표 지점에 이만큼 가까워지면 표시를 숨김
	constexpr float MarkerHideDistance = 800.f;

	// MarkerHeight: 지면에서 목표 표시를 띄우는 높이
	constexpr float MarkerHeight = 250.f;

	// FirstShotCount: 다음 목표로 넘어가기까지 쏴봐야 하는 마법 횟수
	constexpr int32 FirstShotCount = 2;
}

// 안내 화면을 코드로 만듦: 위쪽 가운데 목표, 그 아래 알림, 왼쪽 아래 조작 안내, 목표 지점 표시
TSharedRef<SWidget> UTutorialGuideWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// 목표 패널
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObjectivePanel"));
		Panel->SetBrushColor(FLinearColor(0.02f, 0.01f, 0.05f, 0.65f));
		Panel->SetPadding(FMargin(22.f, 10.f));
		ObjectivePanel = Panel;

		UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(PanelBox);

		StepCountText = MakeText(12.f, FLinearColor(0.8f, 0.72f, 1.f));
		PanelBox->AddChildToVerticalBox(StepCountText);
		ObjectiveText = MakeText(20.f, FLinearColor::White);
		PanelBox->AddChildToVerticalBox(ObjectiveText);

		UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.f));
		PanelSlot->SetPosition(FVector2D(0.f, 24.f));
		PanelSlot->SetAutoSize(true);

		// 화면 알림
		AnnouncementText = MakeText(24.f, FLinearColor(1.f, 0.86f, 0.45f));
		AnnouncementText->SetJustification(ETextJustify::Center);
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* AnnouncementSlot = Root->AddChildToCanvas(AnnouncementText);
		AnnouncementSlot->SetAnchors(FAnchors(0.5f, 0.24f));
		AnnouncementSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		AnnouncementSlot->SetAutoSize(true);

		// 조작 안내
		UTextBlock* ControlsText = MakeText(13.f, FLinearColor(1.f, 1.f, 1.f, 0.9f));
		ControlsText->SetText(NSLOCTEXT("Guide", "Controls",
			"WASD 이동   Shift 달리기   Space 점프\n"
			"좌클릭 마법   우클릭 조준   Q 락온\n"
			"Y 소환   4+1 소환수 모으기   4+2 내 대상 공격\n"
			"R 명상(MP 회복)   Esc 두 번 종료"));
		UCanvasPanelSlot* ControlsSlot = Root->AddChildToCanvas(ControlsText);
		ControlsSlot->SetAnchors(FAnchors(0.f, 1.f));
		ControlsSlot->SetAlignment(FVector2D(0.f, 1.f));
		ControlsSlot->SetPosition(FVector2D(24.f, -20.f));
		ControlsSlot->SetAutoSize(true);

		// 목표 지점 표시: 45도 돌린 네모(마름모)와 남은 거리
		Marker = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Marker"));
		UCanvasPanelSlot* MarkerSlot = Root->AddChildToCanvas(Marker);
		MarkerSlot->SetAnchors(FAnchors(0.f, 0.f));
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerSlot->SetSize(FVector2D(90.f, 60.f));

		UImage* Diamond = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Diamond->SetColorAndOpacity(FLinearColor(1.f, 0.82f, 0.3f, 0.95f));
		Diamond->SetRenderTransformAngle(45.f);
		UCanvasPanelSlot* DiamondSlot = Marker->AddChildToCanvas(Diamond);
		DiamondSlot->SetAnchors(FAnchors(0.5f, 0.f));
		DiamondSlot->SetAlignment(FVector2D(0.5f, 0.f));
		DiamondSlot->SetPosition(FVector2D(0.f, 6.f));
		DiamondSlot->SetSize(FVector2D(16.f, 16.f));

		MarkerDistanceText = MakeText(13.f, FLinearColor::White);
		MarkerDistanceText->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot* DistanceSlot = Marker->AddChildToCanvas(MarkerDistanceText);
		DistanceSlot->SetAnchors(FAnchors(0.5f, 0.f));
		DistanceSlot->SetAlignment(FVector2D(0.5f, 0.f));
		DistanceSlot->SetPosition(FVector2D(0.f, 30.f));
		DistanceSlot->SetAutoSize(true);
	}

	return Super::RebuildWidget();
}


UTextBlock* UTutorialGuideWidget::MakeText(float FontSize, const FLinearColor& Color)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	return Text;
}


// 시작: 알림을 구독하고, Tutorial 맵이면 목표 안내를 켬
void UTutorialGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameAnnouncementSubsystem* Announcements = GetWorld() ? GetWorld()->GetSubsystem<UGameAnnouncementSubsystem>() : nullptr)
	{
		Announcements->OnAnnouncement.AddDynamic(this, &UTutorialGuideWidget::HandleAnnouncement);
	}

	bShowObjectives = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("Tutorial"));
	ObjectivePanel->SetVisibility(bShowObjectives ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Marker->SetVisibility(ESlateVisibility::Collapsed);
	RefreshObjectiveText();
}


void UTutorialGuideWidget::NativeDestruct()
{
	if (UGameAnnouncementSubsystem* Announcements = GetWorld() ? GetWorld()->GetSubsystem<UGameAnnouncementSubsystem>() : nullptr)
	{
		Announcements->OnAnnouncement.RemoveDynamic(this, &UTutorialGuideWidget::HandleAnnouncement);
	}
	Super::NativeDestruct();
}


void UTutorialGuideWidget::HandleAnnouncement(const FText& Message, float Duration)
{
	AnnouncementText->SetText(Message);
	AnnouncementText->SetVisibility(ESlateVisibility::HitTestInvisible);
	AnnouncementHideTime = GetWorld()->GetTimeSeconds() + Duration;
}


void UTutorialGuideWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AnnouncementText->GetVisibility() != ESlateVisibility::Collapsed && GetWorld()->GetTimeSeconds() >= AnnouncementHideTime)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bShowObjectives)
	{
		UpdateStep();
		UpdateMarker();
	}
}


ARabbit* UTutorialGuideWidget::GetRabbit() const
{
	return Cast<ARabbit>(GetOwningPlayerPawn());
}


UMonsterCompanionComponent* UTutorialGuideWidget::GetCompanion() const
{
	const ARabbit* Rabbit = GetRabbit();
	return Rabbit ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
}


AActor* UTutorialGuideWidget::FindDroppedCard() const
{
	const ARabbit* Rabbit = GetRabbit();
	if (!Rabbit) return nullptr;

	AActor* NearestCard = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AMonsterCardPickup> It(GetWorld()); It; ++It)
	{
		const float DistSq = FVector::DistSquared(It->GetActorLocation(), Rabbit->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestCard = *It;
		}
	}
	return NearestCard;
}


void UTutorialGuideWidget::UpdateStep()
{
	const ARabbit* Rabbit = GetRabbit();
	const UMonsterCompanionComponent* Companion = GetCompanion();
	if (!Rabbit || !Companion) return;

	const auto IsNear = [Rabbit](const FVector& Zone, float Radius)
	{
		return FVector::DistSquared2D(Rabbit->GetActorLocation(), Zone) <= FMath::Square(Radius);
	};
	const bool bHasCard = Companion->RegisteredMonsters.Num() > 0;

	// 안내보다 앞서간 플레이어는 맞는 목표로 건너뜀
	ETutorialStep NextStep = CurrentStep;
	if (CurrentStep < ETutorialStep::HuntTogether && Companion->AreSummonsActive())
	{
		NextStep = ETutorialStep::HuntTogether;
	}
	else if (CurrentStep < ETutorialStep::Summon && bHasCard)
	{
		NextStep = ETutorialStep::Summon;
	}
	else if (CurrentStep < ETutorialStep::PickUpCard && FindDroppedCard())
	{
		NextStep = ETutorialStep::PickUpCard;
	}
	else
	{
		switch (CurrentStep)
		{
		case ETutorialStep::MoveToJump:
			if (IsNear(TutorialZones::Jump, TutorialZones::JumpArriveRadius)) NextStep = ETutorialStep::MoveToAim;
			break;
		case ETutorialStep::MoveToAim:
			if (IsNear(TutorialZones::Aim, TutorialZones::AimArriveRadius)) NextStep = ETutorialStep::FirstShot;
			break;
		case ETutorialStep::FirstShot:
			if (Rabbit->GetAttackCount() - StepStartAttackCount >= TutorialZones::FirstShotCount) NextStep = ETutorialStep::HuntFirst;
			break;
		case ETutorialStep::HuntTogether:
			if (Companion->WildKillCount - StepStartKillCount >= HuntTogetherKills) NextStep = ETutorialStep::GoToExit;
			break;
		case ETutorialStep::GoToExit:
			if (IsNear(TutorialZones::Exit, TutorialZones::ExitArriveRadius)) NextStep = ETutorialStep::Done;
			break;
		default:
			break;
		}
	}

	if (NextStep != CurrentStep)
	{
		EnterStep(NextStep);
	}

	// 처치 수는 목표가 그대로여도 바뀌므로 매번 갱신
	if (CurrentStep == ETutorialStep::HuntTogether)
	{
		RefreshObjectiveText();
	}
}


void UTutorialGuideWidget::EnterStep(ETutorialStep NewStep)
{
	CurrentStep = NewStep;

	if (ARabbit* Rabbit = GetRabbit())
	{
		StepStartAttackCount = Rabbit->GetAttackCount();
		if (!Rabbit->IsDead())
		{
			Rabbit->SetRespawnPoint();
		}
		if (Rabbit->GuideStepSound)
		{
			UGameplayStatics::PlaySound2D(this, Rabbit->GuideStepSound);
		}
	}
	if (const UMonsterCompanionComponent* Companion = GetCompanion())
	{
		StepStartKillCount = Companion->WildKillCount;
	}

	RefreshObjectiveText();
}


void UTutorialGuideWidget::RefreshObjectiveText()
{
	FText Objective;
	switch (CurrentStep)
	{
	case ETutorialStep::MoveToJump:
		Objective = NSLOCTEXT("Guide", "StepMoveToJump", "WASD로 움직여 노란 표시까지 가세요 (Shift로 달리기)");
		break;
	case ETutorialStep::MoveToAim:
		Objective = NSLOCTEXT("Guide", "StepMoveToAim", "Space로 점프해 발판을 넘고 다음 표시로 가세요");
		break;
	case ETutorialStep::FirstShot:
		Objective = NSLOCTEXT("Guide", "StepFirstShot", "좌클릭으로 마법을 두 번 쏴보세요 (우클릭을 누르면 조준)");
		break;
	case ETutorialStep::HuntFirst:
		Objective = NSLOCTEXT("Guide", "StepHuntFirst", "표시된 곳의 박쥐를 잡으세요 (Q를 누르면 락온돼서 마법이 따라갑니다)");
		break;
	case ETutorialStep::PickUpCard:
		Objective = NSLOCTEXT("Guide", "StepPickUpCard", "빛나는 카드에 다가가 주우세요");
		break;
	case ETutorialStep::Summon:
		Objective = NSLOCTEXT("Guide", "StepSummon", "카드를 얻었습니다! Y를 눌러 소환하세요");
		break;
	case ETutorialStep::HuntTogether:
	{
		const int32 Kills = FMath::Clamp(GetCompanion() ? GetCompanion()->WildKillCount - StepStartKillCount : 0, 0, HuntTogetherKills);
		Objective = FText::Format(NSLOCTEXT("Guide", "StepHuntTogether", "소환수와 함께 표시된 곳의 박쥐를 잡으세요 ({0}/{1})\n소환수가 쓰러지면 Y로 다시 부를 수 있습니다"),
			FText::AsNumber(Kills), FText::AsNumber(HuntTogetherKills));
		break;
	}
	case ETutorialStep::GoToExit:
		Objective = NSLOCTEXT("Guide", "StepGoToExit", "잘했어요! 표시된 출구로 가세요");
		break;
	case ETutorialStep::Done:
		Objective = NSLOCTEXT("Guide", "StepDone", "데모 끝! 쓰러뜨린 적이, 동료가 됐습니다.\n계속 사냥하거나 Esc를 두 번 눌러 종료하세요");
		break;
	}

	ObjectiveText->SetText(Objective);
	StepCountText->SetText(FText::Format(NSLOCTEXT("Guide", "StepCount", "목표 {0} / {1}"),
		FText::AsNumber(static_cast<int32>(CurrentStep) + 1), FText::AsNumber(static_cast<int32>(ETutorialStep::Done) + 1)));
}


bool UTutorialGuideWidget::GetObjectiveLocation(FVector& OutLocation) const
{
	switch (CurrentStep)
	{
	case ETutorialStep::MoveToJump:
		OutLocation = TutorialZones::Jump;
		return true;
	case ETutorialStep::MoveToAim:
		OutLocation = TutorialZones::Aim;
		return true;
	case ETutorialStep::HuntFirst:
		OutLocation = TutorialZones::FirstHunt;
		return true;
	case ETutorialStep::PickUpCard:
		if (const AActor* Card = FindDroppedCard())
		{
			OutLocation = Card->GetActorLocation();
			return true;
		}
		return false;
	case ETutorialStep::HuntTogether:
		OutLocation = TutorialZones::Summon;
		return true;
	case ETutorialStep::GoToExit:
		OutLocation = TutorialZones::Exit;
		return true;
	default:
		return false;
	}
}


void UTutorialGuideWidget::UpdateMarker()
{
	APlayerController* PC = GetOwningPlayer();
	const ARabbit* Rabbit = GetRabbit();
	FVector Target;
	if (!PC || !Rabbit || !GetObjectiveLocation(Target) || FVector::Dist2D(Rabbit->GetActorLocation(), Target) < TutorialZones::MarkerHideDistance)
	{
		Marker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Target.Z += TutorialZones::MarkerHeight;
	Marker->SetVisibility(ESlateVisibility::HitTestInvisible);

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector ToTargetInCamera = CameraRotation.UnrotateVector(Target - CameraLocation);

	const FVector2D ScreenSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this).GetLocalSize();
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	const bool bInFront = ToTargetInCamera.X > 0.f
		&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Target, ScreenPosition, false);
	if (!bInFront)
	{
		// 카메라 뒤쪽이면 투영값을 믿을 수 없으므로, 그쪽 방향으로 화면 밖까지 밀어서 아래에서 가장자리에 붙임
		const FVector2D Direction = FVector2D(ToTargetInCamera.Y, -ToTargetInCamera.Z).GetSafeNormal();
		ScreenPosition = ScreenSize * 0.5f + Direction * ScreenSize.Size();
	}

	const FVector2D Margin(60.f, 90.f);
	ScreenPosition.X = FMath::Clamp(ScreenPosition.X, Margin.X, ScreenSize.X - Margin.X);
	ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, Margin.Y, ScreenSize.Y - Margin.Y);

	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
	{
		MarkerSlot->SetPosition(ScreenPosition);
	}

	const int32 Meters = FMath::RoundToInt(FVector::Dist(Rabbit->GetActorLocation(), Target) / 100.f);
	MarkerDistanceText->SetText(FText::Format(NSLOCTEXT("Guide", "MarkerDistance", "{0}m"), FText::AsNumber(Meters)));
}
