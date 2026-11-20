// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ItemPickupFeedWidget.h"
#include "UI/InventoryStyle.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> UItemPickupFeedWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);

		List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
		UCanvasPanelSlot* ListSlot = Root->AddChildToCanvas(List);
		ListSlot->SetAnchors(FAnchors(1.f, 0.62f));
		ListSlot->SetAlignment(FVector2D(1.f, 1.f));
		ListSlot->SetPosition(FVector2D(-28.f, 0.f));
		ListSlot->SetAutoSize(true);
	}
	return Super::RebuildWidget();
}


void UItemPickupFeedWidget::BindInventory(UInventoryComponent* Inventory)
{
	if (Inventory)
	{
		Inventory->OnItemAdded.AddUniqueDynamic(this, &UItemPickupFeedWidget::HandleItemAdded);
	}
}


void UItemPickupFeedWidget::HandleItemAdded(UItemDefinition* Item, int32 Count)
{
	if (!Item || !List) return;

	// 같은 아이템 줄이 아직 떠 있으면 개수만 더하고 시간을 새로 시작
	for (FLine& Line : Lines)
	{
		if (Line.Item == Item && Line.Text.IsValid())
		{
			Line.Count += Count;
			Line.TimeLeft = LineLifetime;
			Line.Text->SetText(FText::Format(NSLOCTEXT("Inventory", "PickupLine", "+{0}  {1}"), Line.Count, Item->DisplayName));
			if (Line.Row.IsValid())
			{
				Line.Row->SetRenderOpacity(1.f);
			}
			return;
		}
	}

	UBorder* Row = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Row->SetBrush(InventoryStyle::MakeRoundedBrush(FLinearColor(0.03f, 0.04f, 0.07f, 0.72f), FLinearColor::Transparent, 8.f, 0.f));
	Row->SetPadding(FMargin(10.f, 5.f, 14.f, 5.f));

	UHorizontalBox* Box = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Row->SetContent(Box);

	if (Item->Icon)
	{
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetBrushFromTexture(Item->Icon);
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(30.f);
		IconSize->SetHeightOverride(30.f);
		IconSize->SetContent(Icon);
		UHorizontalBoxSlot* IconSlot = Box->AddChildToHorizontalBox(IconSize);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 15;
	Font.OutlineSettings.OutlineSize = 1;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(UItemDefinition::GetRarityColor(Item->Rarity)));
	Text->SetText(FText::Format(NSLOCTEXT("Inventory", "PickupLine", "+{0}  {1}"), Count, Item->DisplayName));
	Box->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);

	UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row);
	RowSlot->SetHorizontalAlignment(HAlign_Right);
	RowSlot->SetPadding(FMargin(0.f, 3.f));

	FLine Line;
	Line.Row = Row;
	Line.Text = Text;
	Line.Item = Item;
	Line.Count = Count;
	Line.TimeLeft = LineLifetime;
	Lines.Add(Line);

	while (Lines.Num() > MaxLines)
	{
		if (Lines[0].Row.IsValid())
		{
			Lines[0].Row->RemoveFromParent();
		}
		Lines.RemoveAt(0);
	}
}


void UItemPickupFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	for (int32 Index = Lines.Num() - 1; Index >= 0; --Index)
	{
		FLine& Line = Lines[Index];
		Line.TimeLeft -= InDeltaTime;
		if (Line.TimeLeft <= 0.f || !Line.Row.IsValid())
		{
			if (Line.Row.IsValid())
			{
				Line.Row->RemoveFromParent();
			}
			Lines.RemoveAt(Index);
			continue;
		}
		Line.Row->SetRenderOpacity(FMath::Clamp(Line.TimeLeft / 0.6f, 0.f, 1.f));
	}
}
