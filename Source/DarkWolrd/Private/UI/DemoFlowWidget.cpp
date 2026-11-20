// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DemoFlowWidget.h"
#include "Character/Rabbit.h"
#include "Companion/MonsterCompanionComponent.h"
#include "UI/TutorialGuideWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// 가운데 정렬된 제목, 부제, 본문, 버튼 목록을 어두운 전체 화면 위에 그림
TSharedRef<SWidget> UDemoFlowWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Overlay"));
		Dim->SetBrushColor(FLinearColor(0.03f, 0.015f, 0.07f, 0.86f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		Overlay = Dim;

		UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim);
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Dim->SetContent(Column);

		HeadlineText = MakeText(54.f, FLinearColor(0.93f, 0.88f, 1.f), Column);
		SubText = MakeText(22.f, FLinearColor(1.f, 0.84f, 0.45f), Column);
		BodyText = MakeText(16.f, FLinearColor(1.f, 1.f, 1.f, 0.85f), Column);
		if (UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(BodyText->Slot))
		{
			BodySlot->SetPadding(FMargin(0.f, 14.f, 0.f, 28.f));
		}

		// 버튼은 폭을 맞춰 세로로 쌓음
		USizeBox* ButtonWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ButtonWidth->SetWidthOverride(320.f);
		UVerticalBoxSlot* ButtonAreaSlot = Column->AddChildToVerticalBox(ButtonWidth);
		ButtonAreaSlot->SetHorizontalAlignment(HAlign_Center);

		UVerticalBox* Buttons = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		ButtonWidth->SetContent(Buttons);

		StartButton = MakeButton(NSLOCTEXT("Demo", "Start", "시작하기"), Buttons);
		RetryButton = MakeButton(NSLOCTEXT("Demo", "Retry", "마지막 목표에서 다시"), Buttons);
		ContinueButton = MakeButton(NSLOCTEXT("Demo", "Continue", "계속 사냥하기"), Buttons);
		RestartButton = MakeButton(NSLOCTEXT("Demo", "Restart", "처음부터 다시"), Buttons);
		QuitButton = MakeButton(NSLOCTEXT("Demo", "Quit", "종료"), Buttons);
	}

	return Super::RebuildWidget();
}


UTextBlock* UDemoFlowWidget::MakeText(float FontSize, const FLinearColor& Color, UVerticalBox* Parent)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(ETextJustify::Center);

	UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(Text);
	TextSlot->SetHorizontalAlignment(HAlign_Center);
	return Text;
}


UButton* UDemoFlowWidget::MakeButton(const FText& Label, UVerticalBox* Parent)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.42f, 0.3f, 0.75f, 1.f));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = 18.f;
	LabelText->SetFont(Font);
	LabelText->SetText(Label);
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LabelText->SetJustification(ETextJustify::Center);
	Button->SetContent(LabelText);

	UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	ButtonSlot->SetPadding(FMargin(0.f, 6.f));
	return Button;
}


// 시작: 버튼을 연결하고, 토끼가 쓰러지면 실패 화면을 띄우도록 구독한 뒤 시작 화면을 보여줌
void UDemoFlowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	StartButton->OnClicked.AddUniqueDynamic(this, &UDemoFlowWidget::StartGame);
	RetryButton->OnClicked.AddUniqueDynamic(this, &UDemoFlowWidget::RetryFromCheckpoint);
	ContinueButton->OnClicked.AddUniqueDynamic(this, &UDemoFlowWidget::ContinueHunting);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UDemoFlowWidget::RestartFromBeginning);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UDemoFlowWidget::QuitDemo);

	if (ARabbit* Rabbit = GetRabbit())
	{
		Rabbit->bAutoRespawn = false;
		Rabbit->OnDied.AddUniqueDynamic(this, &UDemoFlowWidget::HandleRabbitDied);
	}

	ShowScreen(EDemoScreen::Title);
}


void UDemoFlowWidget::NativeDestruct()
{
	if (ARabbit* Rabbit = GetRabbit())
	{
		Rabbit->OnDied.RemoveDynamic(this, &UDemoFlowWidget::HandleRabbitDied);
	}
	Super::NativeDestruct();
}


// 월드 틱마다: 예정된 실패 화면을 띄우고, 튜토리얼 목표를 모두 끝냈으면 클리어 화면을 예약
void UDemoFlowWidget::TickFlow()
{
	const ARabbit* Rabbit = GetRabbit();
	if (Screen != EDemoScreen::None || !Rabbit || !GetWorld() || !Overlay) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (PendingFailTime > 0.f && Now >= PendingFailTime)
	{
		PendingFailTime = 0.f;
		ShowScreen(EDemoScreen::Failed);
		return;
	}

	const UTutorialGuideWidget* Guide = Rabbit->GetGuideWidget();
	if (!bCleared && Guide && Guide->GetCurrentStep() == ETutorialStep::Done)
	{
		bCleared = true;
		PendingClearTime = Now + ClearDelay;
	}
	if (PendingClearTime > 0.f && Now >= PendingClearTime && !Rabbit->IsDead())
	{
		PendingClearTime = 0.f;
		ShowScreen(EDemoScreen::Cleared);
	}
}


void UDemoFlowWidget::HandleRabbitDied()
{
	++DeathCount;
	if (GetWorld())
	{
		PendingFailTime = GetWorld()->GetTimeSeconds() + FailDelay;
	}
}


void UDemoFlowWidget::ShowScreen(EDemoScreen NewScreen)
{
	Screen = NewScreen;
	APlayerController* PC = GetOwningPlayer();

	if (NewScreen == EDemoScreen::None)
	{
		Overlay->SetVisibility(ESlateVisibility::Collapsed);
		UGameplayStatics::SetGamePaused(this, false);
		if (ARabbit* Rabbit = PC ? Cast<ARabbit>(PC->GetPawn()) : nullptr)
		{
			// 쿼터뷰면 조준용 커서를 보이고, 3인칭이면 숨김
			Rabbit->ApplyGameplayInputMode();
		}
		else if (PC)
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
		return;
	}

	const auto Show = [](UWidget* Widget, bool bVisible)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	};
	Show(StartButton, NewScreen == EDemoScreen::Title);
	Show(RetryButton, NewScreen == EDemoScreen::Failed);
	Show(ContinueButton, NewScreen == EDemoScreen::Cleared);
	Show(RestartButton, NewScreen != EDemoScreen::Title);
	Show(QuitButton, true);

	UButton* FirstButton = StartButton;
	switch (NewScreen)
	{
	case EDemoScreen::Title:
		HeadlineText->SetText(NSLOCTEXT("Demo", "TitleHeadline", "DarkWolrd"));
		SubText->SetText(NSLOCTEXT("Demo", "TitleSub", "쓰러뜨린 적이, 동료가 된다"));
		BodyText->SetText(NSLOCTEXT("Demo", "TitleBody", "데모 v1 · 튜토리얼\n마법으로 박쥐를 쓰러뜨려 카드를 얻고, 소환해서 함께 싸우세요"));
		break;

	case EDemoScreen::Failed:
		HeadlineText->SetText(NSLOCTEXT("Demo", "FailedHeadline", "쓰러졌습니다"));
		SubText->SetText(NSLOCTEXT("Demo", "FailedSub", "마지막으로 달성한 목표에서 다시 시작할 수 있어요"));
		BodyText->SetText(NSLOCTEXT("Demo", "FailedBody", "팁: Q로 락온하면 마법이 적을 따라갑니다. MP가 부족하면 R로 명상하세요"));
		FirstButton = RetryButton;
		break;

	case EDemoScreen::Cleared:
	{
		const float PlaySeconds = GetWorld() ? FMath::Max(GetWorld()->GetTimeSeconds() - PlayStartTime, 0.f) : 0.f;
		const int32 Minutes = FMath::FloorToInt(PlaySeconds / 60.f);
		const int32 Seconds = FMath::FloorToInt(PlaySeconds) % 60;
		const ARabbit* Rabbit = GetRabbit();
		const UMonsterCompanionComponent* Companion = Rabbit ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
		const int32 Kills = Companion ? Companion->WildKillCount : 0;

		HeadlineText->SetText(NSLOCTEXT("Demo", "ClearedHeadline", "데모 클리어!"));
		SubText->SetText(NSLOCTEXT("Demo", "ClearedSub", "쓰러뜨린 적이, 동료가 됐습니다"));
		BodyText->SetText(FText::Format(NSLOCTEXT("Demo", "ClearedBody", "플레이 시간 {0}분 {1}초 · 처치 {2} · 쓰러짐 {3}"),
			FText::AsNumber(Minutes), FText::AsNumber(Seconds), FText::AsNumber(Kills), FText::AsNumber(DeathCount)));
		FirstButton = ContinueButton;
		break;
	}

	default:
		break;
	}

	Overlay->SetVisibility(ESlateVisibility::Visible);
	UGameplayStatics::SetGamePaused(this, true);
	if (PC)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(FirstButton->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}


void UDemoFlowWidget::StartGame()
{
	if (Screen != EDemoScreen::Title) return;
	ShowScreen(EDemoScreen::None);
	PlayStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}


void UDemoFlowWidget::RetryFromCheckpoint()
{
	if (Screen != EDemoScreen::Failed) return;
	ShowScreen(EDemoScreen::None);
	if (ARabbit* Rabbit = GetRabbit())
	{
		Rabbit->RespawnNow();
	}
}


void UDemoFlowWidget::ContinueHunting()
{
	if (Screen != EDemoScreen::Cleared) return;
	ShowScreen(EDemoScreen::None);
}


void UDemoFlowWidget::RestartFromBeginning()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}


void UDemoFlowWidget::QuitDemo()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}


ARabbit* UDemoFlowWidget::GetRabbit() const
{
	return Cast<ARabbit>(GetOwningPlayerPawn());
}
