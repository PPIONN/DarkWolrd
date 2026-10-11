// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/MonsterHPBarWidget.h"
#include "Monster/BaseMonster.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"

// 디자이너에서 만든 위젯이 없으면(이 C++ 클래스를 그대로 쓰는 경우) 잔상 바 위에 HP 바를 겹친 구조를 코드로 만듦
TSharedRef<SWidget> UMonsterHPBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		TrailBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("TrailBar"));
		HPBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HPBar"));

		for (UProgressBar* Bar : { TrailBar, HPBar })
		{
			UOverlaySlot* BarSlot = Root->AddChildToOverlay(Bar);
			BarSlot->SetHorizontalAlignment(HAlign_Fill);
			BarSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// 잔상 바: 반투명 검은 바탕 + 밝은 잔상. HP 바: 바탕을 투명하게 해서 뒤의 잔상이 보이게 함
		FProgressBarStyle TrailStyle = TrailBar->GetWidgetStyle();
		TrailStyle.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.65f));
		TrailBar->SetWidgetStyle(TrailStyle);
		TrailBar->SetFillColorAndOpacity(FLinearColor(1.f, 0.92f, 0.75f));

		FProgressBarStyle HPStyle = HPBar->GetWidgetStyle();
		HPStyle.BackgroundImage.TintColor = FSlateColor(FLinearColor::Transparent);
		HPBar->SetWidgetStyle(HPStyle);
	}

	return Super::RebuildWidget();
}


void UMonsterHPBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HPBar)
	{
		HPBar->SetFillColorAndOpacity(FillColor);
		HPBar->SetPercent(LastPercent);
	}
	if (TrailBar)
	{
		TrailBar->SetPercent(TrailPercent);
	}
}


void UMonsterHPBarWidget::SetMonster(ABaseMonster* InMonster, const FLinearColor& InFillColor)
{
	Monster = InMonster;
	FillColor = InFillColor;
	LastPercent = GetHPPercent();
	TrailPercent = LastPercent;

	if (HPBar)
	{
		HPBar->SetFillColorAndOpacity(FillColor);
		HPBar->SetPercent(LastPercent);
	}
	if (TrailBar)
	{
		TrailBar->SetPercent(TrailPercent);
	}
}


float UMonsterHPBarWidget::GetHPPercent() const
{
	const ABaseMonster* Owner = Monster.Get();
	if (!Owner) return 0.f;

	const float MaxHP = Owner->GetMaxHP();
	return MaxHP > 0.f ? FMath::Clamp(Owner->CurrentHP / MaxHP, 0.f, 1.f) : 0.f;
}


// 매 프레임: HP 바는 즉시 맞추고, 잔상은 피해 직후 잠깐 멈췄다가 천천히 따라 내려감 (회복은 즉시 반영)
void UMonsterHPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!Monster.IsValid()) return;

	const float Percent = GetHPPercent();
	if (Percent < LastPercent)
	{
		TrailHoldTimeLeft = TrailDelay;
	}
	LastPercent = Percent;

	if (Percent >= TrailPercent)
	{
		TrailPercent = Percent;
	}
	else if (TrailHoldTimeLeft > 0.f)
	{
		TrailHoldTimeLeft -= InDeltaTime;
	}
	else
	{
		// HP가 0이 되면(쓰러짐) 바가 사라지기 전에 잔상이 다 빠지도록 빠르게 줄임
		const float Speed = Percent <= 0.f ? TrailSpeed * 2.5f : TrailSpeed;
		TrailPercent = FMath::FInterpConstantTo(TrailPercent, Percent, InDeltaTime, Speed);
	}

	if (HPBar)
	{
		HPBar->SetPercent(Percent);
	}
	if (TrailBar)
	{
		TrailBar->SetPercent(TrailPercent);
	}
}
