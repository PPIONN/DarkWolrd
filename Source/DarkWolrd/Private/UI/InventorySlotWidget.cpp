// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventorySlotWidget.h"
#include "UI/InventoryWidget.h"
#include "UI/InventoryStyle.h"
#include "Inventory/ItemDefinition.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> UInventorySlotWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
		SizeBox->SetWidthOverride(SlotSize);
		SizeBox->SetHeightOverride(SlotSize);
		WidgetTree->RootWidget = SizeBox;

		Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetPadding(FMargin(0.f));
		SizeBox->SetContent(Frame);

		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Frame->SetContent(Overlay);

		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(Icon);
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
		IconSlot->SetPadding(FMargin(SlotSize * 0.06f));

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.Size = FMath::RoundToInt(SlotSize * 0.15f);
			NameText->SetFont(Font);
		}
		NameText->SetJustification(ETextJustify::Center);
		NameText->SetAutoWrapText(true);
		NameText->SetColorAndOpacity(FSlateColor(InventoryStyle::TextMain));
		UOverlaySlot* NameSlot = Overlay->AddChildToOverlay(NameText);
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(5.f, 0.f));

		RarityBar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RarityBar"));
		RarityBar->SetBrush(InventoryStyle::MakeRoundedBrush(FLinearColor::White, FLinearColor::Transparent, 1.5f, 0.f));
		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarSize->SetHeightOverride(3.f);
		BarSize->SetContent(RarityBar);
		UOverlaySlot* BarSlot = Overlay->AddChildToOverlay(BarSize);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Bottom);
		BarSlot->SetPadding(FMargin(SlotSize * 0.18f, 0.f, SlotSize * 0.18f, 4.f));

		CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Count"));
		{
			FSlateFontInfo Font = CountText->GetFont();
			Font.Size = FMath::RoundToInt(SlotSize * 0.17f);
			Font.OutlineSettings.OutlineSize = 1;
			Font.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.9f);
			CountText->SetFont(Font);
		}
		CountText->SetColorAndOpacity(FSlateColor(InventoryStyle::TextMain));
		UOverlaySlot* CountSlot = Overlay->AddChildToOverlay(CountText);
		CountSlot->SetHorizontalAlignment(HAlign_Right);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		CountSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 6.f));

		RefreshFrame();
	}
	return Super::RebuildWidget();
}


void UInventorySlotWidget::Setup(UInventoryWidget* InOwner, EInventorySlotKind InKind, int32 InIndex, float InSize)
{
	OwnerWidget = InOwner;
	Kind = InKind;
	Index = InIndex;
	SlotSize = InSize;
	if (SizeBox)
	{
		SizeBox->SetWidthOverride(SlotSize);
		SizeBox->SetHeightOverride(SlotSize);
	}
}


void UInventorySlotWidget::ShowItem(UItemDefinition* InItem, int32 InCount, const FText& EmptyLabel)
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		TakeWidget();
	}

	bHasContent = InItem != nullptr && InCount > 0;
	RarityColor = bHasContent ? UItemDefinition::GetRarityColor(InItem->Rarity) : FLinearColor::White;

	UTexture2D* Texture = bHasContent ? InItem->Icon : nullptr;
	if (Texture)
	{
		Icon->SetBrushFromTexture(Texture);
		Icon->SetColorAndOpacity(FLinearColor::White);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Icon->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 아이콘이 없으면 이름 글자로 대신 (빈 장비 칸은 부위 이름을 흐리게)
	if (bHasContent && !Texture)
	{
		NameText->SetText(InItem->DisplayName);
		NameText->SetColorAndOpacity(FSlateColor(RarityColor));
		NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (!bHasContent && !EmptyLabel.IsEmpty())
	{
		NameText->SetText(EmptyLabel);
		NameText->SetColorAndOpacity(FSlateColor(InventoryStyle::TextFaint));
		NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		NameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	CountText->SetText(FText::AsNumber(InCount));
	CountText->SetVisibility(bHasContent && InCount > 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RarityBar->SetVisibility(bHasContent && InItem->Rarity != EItemRarity::Common ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RarityBar->SetBrushColor(RarityColor);
	RefreshFrame();
}


void UInventorySlotWidget::ShowCard(const FText& Name, const FText& Badge, bool bRegistered)
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		TakeWidget();
	}

	bHasContent = true;
	RarityColor = bRegistered ? InventoryStyle::CardGold : InventoryStyle::TextDim;
	Icon->SetVisibility(ESlateVisibility::Collapsed);
	NameText->SetText(Name);
	NameText->SetColorAndOpacity(FSlateColor(RarityColor));
	NameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	CountText->SetText(Badge);
	CountText->SetVisibility(Badge.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	RarityBar->SetBrushColor(RarityColor);
	RarityBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshFrame();
}


void UInventorySlotWidget::SetSelected(bool bInSelected)
{
	if (bSelected != bInSelected)
	{
		bSelected = bInSelected;
		RefreshFrame();
	}
}


// 칸 테두리: 기본은 흐린 남색, 아이템이 있으면 희귀도 색을 섞고, 선택·마우스 올림·끌어서 올림이면 밝은 금색
void UInventorySlotWidget::RefreshFrame()
{
	if (!Frame) return;

	const bool bFiller = Kind == EInventorySlotKind::Filler;
	FLinearColor Fill = (bHovered || bDragOver) && !bFiller ? InventoryStyle::SlotFillHover : InventoryStyle::SlotFill;
	if (bFiller)
	{
		Fill.A = 0.45f;
	}
	FLinearColor Outline = bHasContent ? FMath::Lerp(InventoryStyle::SlotOutline, RarityColor, 0.55f) : InventoryStyle::SlotOutline;
	float Width = 1.5f;
	if (bSelected || bDragOver)
	{
		Outline = InventoryStyle::Accent;
		Width = 2.5f;
	}
	Frame->SetBrush(InventoryStyle::MakeRoundedBrush(Fill, bFiller ? FLinearColor(0.2f, 0.24f, 0.33f, 0.4f) : Outline, 8.f, Width));
}


FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UInventoryWidget* Owner = OwnerWidget.Get();
	if (!Owner || Kind == EInventorySlotKind::Filler) return FReply::Handled();

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		Owner->OnSlotActivated(this);
		return FReply::Handled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Owner->OnSlotClicked(this);
		if (bHasContent && Kind != EInventorySlotKind::Card)
		{
			return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
		}
	}
	return FReply::Handled();
}


FReply UInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (UInventoryWidget* Owner = OwnerWidget.Get())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Kind != EInventorySlotKind::Filler)
		{
			Owner->OnSlotActivated(this);
		}
	}
	return FReply::Handled();
}


void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UInventoryWidget* Owner = OwnerWidget.Get();
	if (!Owner || !bHasContent) return;

	UInventoryDragOperation* Operation = NewObject<UInventoryDragOperation>(this);
	Operation->SourceKind = Kind;
	Operation->SourceIndex = Index;
	Operation->Pivot = EDragPivot::CenterCenter;

	// 끄는 동안 커서에 붙는 그림: 같은 내용의 칸을 하나 더 만들어 반투명하게
	int32 DragCount = 0;
	UItemDefinition* DragItem = Owner->GetSlotItem(Kind, Index, DragCount);
	UInventorySlotWidget* Visual = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), UInventorySlotWidget::StaticClass());
	if (Visual && DragItem)
	{
		Visual->Setup(nullptr, Kind, Index, SlotSize);
		Visual->ShowItem(DragItem, DragCount);
		Visual->SetRenderOpacity(0.85f);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Operation->DefaultDragVisual = Visual;
	}
	OutOperation = Operation;
}


bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bDragOver = false;
	RefreshFrame();
	UInventoryWidget* Owner = OwnerWidget.Get();
	UInventoryDragOperation* Operation = Cast<UInventoryDragOperation>(InOperation);
	return Owner && Operation && Owner->HandleDrop(Operation, this);
}


void UInventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (Kind != EInventorySlotKind::Filler && Kind != EInventorySlotKind::Card)
	{
		bDragOver = true;
		RefreshFrame();
	}
}


void UInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bDragOver = false;
	RefreshFrame();
}


void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	RefreshFrame();
	if (UInventoryWidget* Owner = OwnerWidget.Get())
	{
		Owner->OnSlotHovered(this);
	}
}


void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	RefreshFrame();
	if (UInventoryWidget* Owner = OwnerWidget.Get())
	{
		Owner->OnSlotUnhovered(this);
	}
}
