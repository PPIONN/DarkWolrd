// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DamageNumberWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

// 디자이너에서 만든 위젯이 없으면 텍스트 하나를 루트로 만듦
TSharedRef<SWidget> UDamageNumberWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
		DamageText->SetJustification(ETextJustify::Center);
		WidgetTree->RootWidget = DamageText;
	}

	return Super::RebuildWidget();
}


void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyToText();
}


void UDamageNumberWidget::SetDamage(float Damage, const FLinearColor& Color, int32 FontSize)
{
	DamageValue = Damage;
	TextColor = Color;
	TextSize = FontSize;
	ApplyToText();
}


// 굵은 글씨 + 검은 외곽선으로 어떤 배경에서도 읽히게 함
void UDamageNumberWidget::ApplyToText()
{
	if (!DamageText) return;

	DamageText->SetText(FText::AsNumber(FMath::Max(1, FMath::RoundToInt(DamageValue))));
	DamageText->SetColorAndOpacity(FSlateColor(TextColor));

	FSlateFontInfo Font = DamageText->GetFont();
	Font.Size = TextSize;
	Font.TypefaceFontName = TEXT("Bold");
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
	DamageText->SetFont(Font);
}
