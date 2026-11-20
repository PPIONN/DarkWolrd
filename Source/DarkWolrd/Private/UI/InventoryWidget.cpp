// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventoryWidget.h"
#include "UI/InventoryStyle.h"
#include "Character/Rabbit.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Stats/CharacterStatComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Monster/BaseMonster.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"

namespace InventoryLayout
{
	constexpr int32 Columns = 7;
	constexpr int32 Rows = 6;
	constexpr float SlotSize = 76.f;
	constexpr float EquipSlotSize = 76.f;
	constexpr float PreviewWidth = 300.f;
	constexpr float PreviewHeight = 400.f;
	constexpr int32 PreviewTextureWidth = 480;
	constexpr int32 PreviewTextureHeight = 640;
	constexpr float PreviewFOV = 28.f;
	constexpr float PreviewLightCandelas = 12.f;
	constexpr float PreviewExposureBias = 0.f;

	// 장비 칸 화면 배치: 왼쪽 열 머리·몸·신발, 오른쪽 열 무기·장신구·장신구 (번호는 InventoryComponent의 장비 칸 번호)
	const int32 LeftColumn[] = { 1, 2, 3 };
	const int32 RightColumn[] = { 0, 4, 5 };
}

enum EStatRow { StatLevel, StatHP, StatMP, StatMagic, StatInt, StatVit, StatDefense, StatAttackSpeed, StatMoveSpeed, StatMoney, StatRowCount };


// ─── 화면 만들기 ───────────────────────────────────────

UTextBlock* UInventoryWidget::MakeText(float Size, const FLinearColor& Color, const FText& Text)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Block->GetFont();
	Font.Size = FMath::RoundToInt(Size);
	Block->SetFont(Font);
	Block->SetColorAndOpacity(FSlateColor(Color));
	Block->SetText(Text);
	return Block;
}


UBorder* UInventoryWidget::MakePanel(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, const FMargin& InPadding)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrush(InventoryStyle::MakeRoundedBrush(Fill, Outline, Radius, Outline.A > 0.f ? 1.f : 0.f));
	Panel->SetPadding(InPadding);
	return Panel;
}


UButton* UInventoryWidget::MakeTabButton(const FText& Label, UTextBlock*& OutLabel)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	OutLabel = MakeText(15.f, InventoryStyle::TextDim, Label);
	Button->SetContent(OutLabel);
	return Button;
}


UInventorySlotWidget* UInventoryWidget::MakeSlot(EInventorySlotKind Kind, int32 Index, float Size)
{
	UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), UInventorySlotWidget::StaticClass());
	SlotWidget->Setup(this, Kind, Index, Size);
	return SlotWidget;
}


void UInventoryWidget::AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock*& OutValue, UTextBlock*& OutBonus)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* LabelText = MakeText(14.f, InventoryStyle::TextDim, Label);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	OutValue = MakeText(15.f, InventoryStyle::TextMain);
	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue);
	ValueSlot->SetVerticalAlignment(VAlign_Center);

	OutBonus = MakeText(13.f, InventoryStyle::Bonus);
	UHorizontalBoxSlot* BonusSlot = Row->AddChildToHorizontalBox(OutBonus);
	BonusSlot->SetVerticalAlignment(VAlign_Center);
	BonusSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));

	UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Row);
	RowSlot->SetPadding(FMargin(0.f, 2.f));
}


TSharedRef<SWidget> UInventoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		SetIsFocusable(true);
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 뒤의 게임 화면을 흐리고 어둡게
		UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("Blur"));
		Blur->SetBlurStrength(6.f);
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
		Dim->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.03f, 0.55f));
		Blur->SetContent(Dim);
		UCanvasPanelSlot* BlurSlot = Root->AddChildToCanvas(Blur);
		BlurSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BlurSlot->SetOffsets(FMargin(0.f));

		UHorizontalBox* Main = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Main"));
		UCanvasPanelSlot* MainSlot = Root->AddChildToCanvas(Main);
		MainSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MainSlot->SetAlignment(FVector2D(0.5f, 0.52f));
		MainSlot->SetAutoSize(true);

		// ── 왼쪽: 아이템 ──
		UBorder* LeftPanel = MakePanel(InventoryStyle::PanelFill, InventoryStyle::PanelOutline, 14.f, FMargin(22.f, 18.f));
		Main->AddChildToHorizontalBox(LeftPanel)->SetVerticalAlignment(VAlign_Fill);
		UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		// 폭을 칸 7개에 딱 맞게 고정: 안 그러면 긴 설명 글이 패널을 넓혀서 칸 사이가 벌어짐
		USizeBox* LeftWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LeftWidth->SetWidthOverride(InventoryLayout::Columns * (InventoryLayout::SlotSize + 8.f));
		LeftWidth->SetContent(Left);
		LeftPanel->SetContent(LeftWidth);

		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(MakeText(26.f, InventoryStyle::TextMain, NSLOCTEXT("Inventory", "Title", "인벤토리")));
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		CapacityText = MakeText(15.f, InventoryStyle::TextDim);
		Header->AddChildToHorizontalBox(CapacityText)->SetVerticalAlignment(VAlign_Center);
		Left->AddChildToVerticalBox(Header)->SetPadding(FMargin(2.f, 0.f, 2.f, 10.f));

		UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FText TabNames[] = {
			NSLOCTEXT("Inventory", "TabAll", "전체"), NSLOCTEXT("Inventory", "TabEquipment", "장비"), NSLOCTEXT("Inventory", "TabConsumable", "소모품"),
			NSLOCTEXT("Inventory", "TabMaterial", "재료"), NSLOCTEXT("Inventory", "TabCard", "카드") };
		for (int32 TabIndex = 0; TabIndex < static_cast<int32>(ETab::Count); ++TabIndex)
		{
			UTextBlock* Label = nullptr;
			UButton* Button = MakeTabButton(TabNames[TabIndex], Label);
			TabButtons.Add(Button);
			TabLabels.Add(Label);
			Tabs->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		}
		TabButtons[0]->OnClicked.AddDynamic(this, &UInventoryWidget::OnTabAll);
		TabButtons[1]->OnClicked.AddDynamic(this, &UInventoryWidget::OnTabEquipment);
		TabButtons[2]->OnClicked.AddDynamic(this, &UInventoryWidget::OnTabConsumable);
		TabButtons[3]->OnClicked.AddDynamic(this, &UInventoryWidget::OnTabMaterial);
		TabButtons[4]->OnClicked.AddDynamic(this, &UInventoryWidget::OnTabCard);

		USpacer* TabSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Tabs->AddChildToHorizontalBox(TabSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* SortLabel = nullptr;
		UButton* SortButton = MakeTabButton(NSLOCTEXT("Inventory", "Sort", "정렬"), SortLabel);
		SortButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnSortClicked);
		{
			FButtonStyle Style = SortButton->GetStyle();
			Style.SetNormal(InventoryStyle::MakeRoundedBrush(InventoryStyle::SubPanelFill, InventoryStyle::SlotOutline, 8.f, 1.f));
			Style.SetHovered(InventoryStyle::MakeRoundedBrush(InventoryStyle::SlotFillHover, InventoryStyle::Accent, 8.f, 1.f));
			Style.SetPressed(InventoryStyle::MakeRoundedBrush(InventoryStyle::SlotFill, InventoryStyle::Accent, 8.f, 1.f));
			Style.SetNormalPadding(FMargin(14.f, 5.f));
			Style.SetPressedPadding(FMargin(14.f, 6.f, 14.f, 4.f));
			SortButton->SetStyle(Style);
		}
		Tabs->AddChildToHorizontalBox(SortButton);
		Left->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

		UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
		Grid->SetSlotPadding(FMargin(4.f));
		for (int32 CellIndex = 0; CellIndex < InventoryLayout::Columns * InventoryLayout::Rows; ++CellIndex)
		{
			UInventorySlotWidget* Cell = MakeSlot(EInventorySlotKind::Inventory, CellIndex, InventoryLayout::SlotSize);
			GridSlots.Add(Cell);
			Grid->AddChildToUniformGrid(Cell, CellIndex / InventoryLayout::Columns, CellIndex % InventoryLayout::Columns);
		}
		Left->AddChildToVerticalBox(Grid);

		// 설명 패널
		UBorder* DetailPanel = MakePanel(InventoryStyle::SubPanelFill, FLinearColor::Transparent, 10.f, FMargin(14.f, 12.f));
		USizeBox* DetailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		DetailSize->SetHeightOverride(170.f);
		DetailSize->SetContent(DetailPanel);
		Left->AddChildToVerticalBox(DetailSize)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

		UHorizontalBox* DetailRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		DetailPanel->SetContent(DetailRow);
		UBorder* IconFrame = MakePanel(InventoryStyle::SlotFill, InventoryStyle::SlotOutline, 10.f, FMargin(8.f));
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(96.f);
		IconSize->SetHeightOverride(96.f);
		IconSize->SetContent(IconFrame);
		DetailIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DetailIcon->SetVisibility(ESlateVisibility::Hidden);
		IconFrame->SetContent(DetailIcon);
		DetailRow->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Top);

		UVerticalBox* DetailText = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBoxSlot* DetailTextSlot = DetailRow->AddChildToHorizontalBox(DetailText);
		DetailTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DetailTextSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
		DetailName = MakeText(20.f, InventoryStyle::TextMain);
		DetailText->AddChildToVerticalBox(DetailName);
		DetailMeta = MakeText(13.f, InventoryStyle::TextDim);
		DetailText->AddChildToVerticalBox(DetailMeta)->SetPadding(FMargin(0.f, 1.f, 0.f, 4.f));
		DetailDescription = MakeText(14.f, InventoryStyle::TextMain);
		DetailDescription->SetAutoWrapText(true);
		DetailText->AddChildToVerticalBox(DetailDescription);
		DetailEffects = MakeText(14.f, InventoryStyle::Bonus);
		DetailText->AddChildToVerticalBox(DetailEffects)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		USpacer* DetailSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		DetailText->AddChildToVerticalBox(DetailSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UHorizontalBox* HintRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		DetailHint = MakeText(12.f, InventoryStyle::TextFaint);
		UHorizontalBoxSlot* HintSlot = HintRow->AddChildToHorizontalBox(DetailHint);
		HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		MessageText = MakeText(13.f, InventoryStyle::Accent);
		HintRow->AddChildToHorizontalBox(MessageText);
		DetailText->AddChildToVerticalBox(HintRow);

		USpacer* MainGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		MainGap->SetSize(FVector2D(18.f, 1.f));
		Main->AddChildToHorizontalBox(MainGap);

		// ── 오른쪽: 캐릭터와 장비 ──
		UBorder* RightPanel = MakePanel(InventoryStyle::PanelFill, InventoryStyle::PanelOutline, 14.f, FMargin(22.f, 18.f));
		Main->AddChildToHorizontalBox(RightPanel)->SetVerticalAlignment(VAlign_Fill);
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		RightPanel->SetContent(Right);

		UHorizontalBox* RightHeader = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		CharacterTitleText = MakeText(22.f, InventoryStyle::TextMain);
		UHorizontalBoxSlot* CharacterSlot = RightHeader->AddChildToHorizontalBox(CharacterTitleText);
		CharacterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CharacterSlot->SetVerticalAlignment(VAlign_Center);
		RightHeader->AddChildToHorizontalBox(MakeText(15.f, InventoryStyle::TextDim, NSLOCTEXT("Inventory", "Equipment", "장비")))->SetVerticalAlignment(VAlign_Center);
		Right->AddChildToVerticalBox(RightHeader)->SetPadding(FMargin(2.f, 4.f, 2.f, 12.f));

		UHorizontalBox* Doll = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		EquipSlots.SetNum(UInventoryComponent::NumEquipSlots);
		const auto MakeEquipColumn = [this](const int32* Indices) -> UVerticalBox*
		{
			UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			for (int32 Row = 0; Row < 3; ++Row)
			{
				const int32 EquipIndex = Indices[Row];
				UInventorySlotWidget* SlotWidget = MakeSlot(EInventorySlotKind::Equipment, EquipIndex, InventoryLayout::EquipSlotSize);
				EquipSlots[EquipIndex] = SlotWidget;
				Column->AddChildToVerticalBox(SlotWidget)->SetHorizontalAlignment(HAlign_Center);
				UTextBlock* Caption = MakeText(12.f, InventoryStyle::TextFaint, UItemDefinition::GetEquipSlotName(UInventoryComponent::GetSlotTypeForEquipIndex(EquipIndex)));
				Caption->SetJustification(ETextJustify::Center);
				UVerticalBoxSlot* CaptionSlot = Column->AddChildToVerticalBox(Caption);
				CaptionSlot->SetHorizontalAlignment(HAlign_Center);
				CaptionSlot->SetPadding(FMargin(0.f, 3.f, 0.f, Row < 2 ? 22.f : 0.f));
			}
			return Column;
		};
		Doll->AddChildToHorizontalBox(MakeEquipColumn(InventoryLayout::LeftColumn))->SetVerticalAlignment(VAlign_Center);

		UOverlay* PreviewOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UBorder* PreviewBack = MakePanel(FLinearColor(0.f, 0.f, 0.f, 1.f), FLinearColor(0.3f, 0.36f, 0.5f, 0.35f), 12.f, FMargin(0.f));
		UOverlaySlot* BackSlot = PreviewOverlay->AddChildToOverlay(PreviewBack);
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetVerticalAlignment(VAlign_Fill);
		PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Preview"));
		UOverlaySlot* ImageSlot = PreviewOverlay->AddChildToOverlay(PreviewImage);
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
		ImageSlot->SetPadding(FMargin(2.f));
		UTextBlock* RotateHint = MakeText(11.f, InventoryStyle::TextFaint, NSLOCTEXT("Inventory", "RotateHint", "끌어서 돌리기"));
		RotateHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* RotateSlot = PreviewOverlay->AddChildToOverlay(RotateHint);
		RotateSlot->SetHorizontalAlignment(HAlign_Center);
		RotateSlot->SetVerticalAlignment(VAlign_Bottom);
		RotateSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		USizeBox* PreviewSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PreviewSize->SetWidthOverride(InventoryLayout::PreviewWidth);
		PreviewSize->SetHeightOverride(InventoryLayout::PreviewHeight);
		PreviewSize->SetContent(PreviewOverlay);
		Doll->AddChildToHorizontalBox(PreviewSize)->SetPadding(FMargin(16.f, 0.f));

		Doll->AddChildToHorizontalBox(MakeEquipColumn(InventoryLayout::RightColumn))->SetVerticalAlignment(VAlign_Center);
		Right->AddChildToVerticalBox(Doll);

		// 능력치: 두 열
		UBorder* StatPanel = MakePanel(InventoryStyle::SubPanelFill, FLinearColor::Transparent, 10.f, FMargin(16.f, 12.f));
		UHorizontalBox* StatColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		StatPanel->SetContent(StatColumns);
		UVerticalBox* StatLeft = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UVerticalBox* StatRight = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		StatColumns->AddChildToHorizontalBox(StatLeft)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		USpacer* StatGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		StatGap->SetSize(FVector2D(28.f, 1.f));
		StatColumns->AddChildToHorizontalBox(StatGap);
		StatColumns->AddChildToHorizontalBox(StatRight)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		const FText StatNames[StatRowCount] = {
			NSLOCTEXT("Inventory", "StatLevel", "레벨"), NSLOCTEXT("Inventory", "StatHP", "HP"), NSLOCTEXT("Inventory", "StatMP", "MP"),
			NSLOCTEXT("Inventory", "StatMagic", "마력"), NSLOCTEXT("Inventory", "StatInt", "지능"), NSLOCTEXT("Inventory", "StatVit", "체력"),
			NSLOCTEXT("Inventory", "StatDefense", "방어력"), NSLOCTEXT("Inventory", "StatAttackSpeed", "공격속도"), NSLOCTEXT("Inventory", "StatMoveSpeed", "이동속도"),
			NSLOCTEXT("Inventory", "StatMoney", "돈") };
		StatValues.SetNum(StatRowCount);
		StatBonuses.SetNum(StatRowCount);
		for (int32 Row = 0; Row < StatRowCount; ++Row)
		{
			AddStatRow(Row < 5 ? StatLeft : StatRight, StatNames[Row], StatValues[Row], StatBonuses[Row]);
		}
		Right->AddChildToVerticalBox(StatPanel)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

		// 아래 조작 안내
		UTextBlock* Hint = MakeText(13.f, InventoryStyle::TextDim, NSLOCTEXT("Inventory", "Controls",
			"좌클릭 선택     우클릭·더블클릭 사용/장착/해제     끌어서 옮기기·장착     I · Tab · Esc 닫기"));
		UCanvasPanelSlot* HintCanvasSlot = Root->AddChildToCanvas(Hint);
		HintCanvasSlot->SetAnchors(FAnchors(0.5f, 1.f));
		HintCanvasSlot->SetAlignment(FVector2D(0.5f, 1.f));
		HintCanvasSlot->SetPosition(FVector2D(0.f, -22.f));
		HintCanvasSlot->SetAutoSize(true);

		SetTab(ETab::All);
		ClearDetails();
	}
	return Super::RebuildWidget();
}


// ─── 열기 / 닫기 ───────────────────────────────────────

void UInventoryWidget::OpenFor(ARabbit* InRabbit)
{
	Rabbit = InRabbit;
	TakeWidget();

	if (UInventoryComponent* Inventory = GetInventory())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}
	HoveredKind = EInventorySlotKind::Filler;
	HoveredIndex = INDEX_NONE;
	MessageTimeLeft = 0.f;
	if (MessageText)
	{
		MessageText->SetText(FText::GetEmpty());
	}
	RefreshAll();
	StartPreview();

	if (bPauseWhileOpen)
	{
		bPausedGame = UGameplayStatics::SetGamePaused(this, true);
	}
}


void UInventoryWidget::CloseInventory()
{
	if (UInventoryComponent* Inventory = GetInventory())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}
	StopPreview();
	bPreviewDragging = false;

	if (bPausedGame)
	{
		UGameplayStatics::SetGamePaused(this, false);
		bPausedGame = false;
	}
}


void UInventoryWidget::NativeDestruct()
{
	StopPreview();
	Super::NativeDestruct();
}


UInventoryComponent* UInventoryWidget::GetInventory() const
{
	return Rabbit.IsValid() ? Rabbit->FindComponentByClass<UInventoryComponent>() : nullptr;
}


void UInventoryWidget::HandleInventoryChanged()
{
	RefreshAll();
}


// ─── 탭 ───────────────────────────────────────

void UInventoryWidget::OnTabAll()			{ SetTab(ETab::All); }
void UInventoryWidget::OnTabEquipment()		{ SetTab(ETab::Equipment); }
void UInventoryWidget::OnTabConsumable()	{ SetTab(ETab::Consumable); }
void UInventoryWidget::OnTabMaterial()		{ SetTab(ETab::Material); }
void UInventoryWidget::OnTabCard()			{ SetTab(ETab::Card); }


void UInventoryWidget::OnSortClicked()
{
	if (UInventoryComponent* Inventory = GetInventory())
	{
		Inventory->SortSlots();
		SelectedKind = EInventorySlotKind::Filler;
		SelectedIndex = INDEX_NONE;
		ShowMessage(NSLOCTEXT("Inventory", "Sorted", "정렬했습니다"));
	}
}


void UInventoryWidget::SetTab(ETab NewTab)
{
	CurrentTab = NewTab;
	for (int32 TabIndex = 0; TabIndex < TabButtons.Num(); ++TabIndex)
	{
		const bool bActive = TabIndex == static_cast<int32>(CurrentTab);
		FButtonStyle Style = TabButtons[TabIndex]->GetStyle();
		const FLinearColor Fill = bActive ? InventoryStyle::TabActiveFill : InventoryStyle::SubPanelFill;
		const FLinearColor Outline = bActive ? InventoryStyle::Accent : InventoryStyle::SlotOutline;
		Style.SetNormal(InventoryStyle::MakeRoundedBrush(Fill, Outline, 8.f, bActive ? 1.5f : 1.f));
		Style.SetHovered(InventoryStyle::MakeRoundedBrush(bActive ? Fill : InventoryStyle::SlotFillHover, InventoryStyle::Accent, 8.f, 1.5f));
		Style.SetPressed(InventoryStyle::MakeRoundedBrush(Fill, InventoryStyle::Accent, 8.f, 1.5f));
		Style.SetNormalPadding(FMargin(16.f, 5.f));
		Style.SetPressedPadding(FMargin(16.f, 6.f, 16.f, 4.f));
		TabButtons[TabIndex]->SetStyle(Style);
		TabLabels[TabIndex]->SetColorAndOpacity(FSlateColor(bActive ? InventoryStyle::Accent : InventoryStyle::TextDim));
	}
	SelectedKind = EInventorySlotKind::Filler;
	SelectedIndex = INDEX_NONE;
	RefreshGrid();
	RefreshDetails();
}


// ─── 갱신 ───────────────────────────────────────

void UInventoryWidget::RefreshAll()
{
	RefreshGrid();
	RefreshEquipment();
	RefreshStats();
	RefreshDetails();
}


void UInventoryWidget::RefreshGrid()
{
	const UInventoryComponent* Inventory = GetInventory();
	const int32 NumCells = GridSlots.Num();

	if (CurrentTab == ETab::Card)
	{
		const UMonsterCompanionComponent* Companion = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
		int32 Cell = 0;
		if (Companion)
		{
			for (const FRegisteredMonsterEntry& Entry : Companion->RegisteredMonsters)
			{
				if (Cell >= NumCells || !Entry.MonsterClass) continue;
				const ABaseMonster* Monster = Entry.MonsterClass->GetDefaultObject<ABaseMonster>();
				GridSlots[Cell]->Setup(this, EInventorySlotKind::Card, Cell, InventoryLayout::SlotSize);
				const FText Badge = Entry.UpgradeCount > 0 ? FText::Format(NSLOCTEXT("Inventory", "CardBadgeUpgraded", "Lv{0} +{1}"), Entry.Level, Entry.UpgradeCount)
					: FText::Format(NSLOCTEXT("Inventory", "CardBadge", "Lv{0}"), Entry.Level);
				GridSlots[Cell]->ShowCard(Monster->GetMonsterDisplayName(), Badge, true);
				++Cell;
			}
			for (const FMonsterCardEntry& Card : Companion->HeldCards)
			{
				if (Cell >= NumCells || !Card.MonsterClass) continue;
				const ABaseMonster* Monster = Card.MonsterClass->GetDefaultObject<ABaseMonster>();
				GridSlots[Cell]->Setup(this, EInventorySlotKind::Card, Cell, InventoryLayout::SlotSize);
				GridSlots[Cell]->ShowCard(Monster->GetMonsterDisplayName(), NSLOCTEXT("Inventory", "CardUnregistered", "미등록"), false);
				++Cell;
			}
		}
		for (; Cell < NumCells; ++Cell)
		{
			GridSlots[Cell]->Setup(this, EInventorySlotKind::Filler, INDEX_NONE, InventoryLayout::SlotSize);
			GridSlots[Cell]->ShowItem(nullptr, 0);
		}
	}
	else
	{
		// 전체 탭은 칸 순서 그대로(끌어서 정리 가능), 나머지 탭은 해당 분류만 앞으로 모아서 보여줌
		TArray<int32> Indices;
		if (Inventory)
		{
			for (int32 SlotIndex = 0; SlotIndex < Inventory->Slots.Num(); ++SlotIndex)
			{
				const FInventorySlot& InvSlot = Inventory->Slots[SlotIndex];
				const bool bMatch = CurrentTab == ETab::All
					|| (!InvSlot.IsEmpty() && ((CurrentTab == ETab::Equipment && InvSlot.Item->Category == EItemCategory::Equipment)
						|| (CurrentTab == ETab::Consumable && InvSlot.Item->Category == EItemCategory::Consumable)
						|| (CurrentTab == ETab::Material && InvSlot.Item->Category == EItemCategory::Material)));
				if (bMatch)
				{
					Indices.Add(SlotIndex);
				}
			}
		}
		for (int32 Cell = 0; Cell < NumCells; ++Cell)
		{
			if (Indices.IsValidIndex(Cell))
			{
				const FInventorySlot& InvSlot = Inventory->Slots[Indices[Cell]];
				GridSlots[Cell]->Setup(this, EInventorySlotKind::Inventory, Indices[Cell], InventoryLayout::SlotSize);
				GridSlots[Cell]->ShowItem(InvSlot.Item, InvSlot.Count);
			}
			else
			{
				GridSlots[Cell]->Setup(this, EInventorySlotKind::Filler, INDEX_NONE, InventoryLayout::SlotSize);
				GridSlots[Cell]->ShowItem(nullptr, 0);
			}
		}
	}

	for (UInventorySlotWidget* Cell : GridSlots)
	{
		Cell->SetSelected(Cell->GetKind() == SelectedKind && Cell->GetIndex() == SelectedIndex && SelectedIndex != INDEX_NONE);
	}

	if (CapacityText)
	{
		CapacityText->SetText(Inventory ? FText::Format(NSLOCTEXT("Inventory", "Capacity", "칸 {0} / {1}"), Inventory->GetUsedSlotCount(), Inventory->Capacity)
			: FText::GetEmpty());
	}
}


void UInventoryWidget::RefreshEquipment()
{
	const UInventoryComponent* Inventory = GetInventory();
	for (int32 EquipIndex = 0; EquipIndex < EquipSlots.Num(); ++EquipIndex)
	{
		UInventorySlotWidget* SlotWidget = EquipSlots[EquipIndex];
		if (!SlotWidget) continue;
		UItemDefinition* Item = Inventory && Inventory->Equipped.IsValidIndex(EquipIndex) ? Inventory->Equipped[EquipIndex] : nullptr;
		// 부위 이름은 칸 아래 글자로 이미 보이므로 빈 칸 안에는 쓰지 않음
		SlotWidget->ShowItem(Item, Item ? 1 : 0);
		SlotWidget->SetSelected(SelectedKind == EInventorySlotKind::Equipment && SelectedIndex == EquipIndex);
	}
}


void UInventoryWidget::RefreshStats()
{
	const UCharacterStatComponent* Stat = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UCharacterStatComponent>() : nullptr;
	if (!Stat || StatValues.Num() < StatRowCount) return;

	const auto SetRow = [this](int32 Row, const FText& Value, const FText& Bonus)
	{
		StatValues[Row]->SetText(Value);
		StatBonuses[Row]->SetText(Bonus);
	};
	const auto BonusInt = [](float Value) { return FMath::RoundToInt(Value) != 0 ? FText::FromString(FString::Printf(TEXT("(%+d)"), FMath::RoundToInt(Value))) : FText::GetEmpty(); };
	const auto Percent = [](float Multiplier) { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Multiplier * 100.f))); };

	CharacterTitleText->SetText(FText::Format(NSLOCTEXT("Inventory", "CharacterTitle", "토끼   Lv.{0}"), Stat->Level));
	SetRow(StatLevel, FText::Format(NSLOCTEXT("Inventory", "LevelValue", "{0}  ({1}/{2})"), Stat->Level, Stat->CurrentEXP, Stat->GetEXPToNextLevel()), FText::GetEmpty());
	SetRow(StatHP, FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Stat->CurrentHP), FMath::RoundToInt(Stat->GetMaxHP()))), BonusInt(Stat->EquipmentBonusHP + Stat->EquipmentBonusVitality * Stat->HPPerVitality));
	SetRow(StatMP, FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(Stat->CurrentMP), FMath::RoundToInt(Stat->GetMaxMP()))), BonusInt(Stat->EquipmentBonusMP));
	SetRow(StatMagic, FText::AsNumber(Stat->GetTotalMagicPower()), BonusInt(Stat->EquipmentBonusMagicPower));
	SetRow(StatInt, FText::AsNumber(Stat->GetTotalIntelligence()), BonusInt(Stat->EquipmentBonusIntelligence));
	SetRow(StatVit, FText::AsNumber(Stat->GetTotalVitality()), BonusInt(Stat->EquipmentBonusVitality));
	SetRow(StatDefense, FText::AsNumber(FMath::RoundToInt(Stat->GetTotalDefense())), BonusInt(Stat->EquipmentBonusDefense));
	SetRow(StatAttackSpeed, Percent(Stat->GetAttackSpeedMultiplier()),
		FMath::IsNearlyZero(Stat->EquipmentBonusAttackSpeed) ? FText::GetEmpty() : FText::FromString(FString::Printf(TEXT("(%+d%%)"), FMath::RoundToInt(Stat->EquipmentBonusAttackSpeed * 100.f))));
	SetRow(StatMoveSpeed, Percent(Stat->GetMoveSpeedMultiplier()),
		FMath::IsNearlyZero(Stat->EquipmentBonusMoveSpeed) ? FText::GetEmpty() : FText::FromString(FString::Printf(TEXT("(%+d%%)"), FMath::RoundToInt(Stat->EquipmentBonusMoveSpeed * 100.f))));
	SetRow(StatMoney, FText::AsNumber(Stat->Money), FText::GetEmpty());
}


void UInventoryWidget::RefreshDetails()
{
	if (HoveredIndex != INDEX_NONE && HoveredKind != EInventorySlotKind::Filler)
	{
		ShowDetailsFor(HoveredKind, HoveredIndex);
	}
	else if (SelectedIndex != INDEX_NONE && SelectedKind != EInventorySlotKind::Filler)
	{
		ShowDetailsFor(SelectedKind, SelectedIndex);
	}
	else
	{
		ClearDetails();
	}
}


void UInventoryWidget::ClearDetails()
{
	if (!DetailName) return;
	DetailIcon->SetVisibility(ESlateVisibility::Hidden);
	DetailName->SetText(NSLOCTEXT("Inventory", "NoSelection", "아이템을 고르세요"));
	DetailName->SetColorAndOpacity(FSlateColor(InventoryStyle::TextDim));
	DetailMeta->SetText(FText::GetEmpty());
	DetailDescription->SetText(NSLOCTEXT("Inventory", "NoSelectionHelp", "칸에 마우스를 올리면 설명이 나옵니다. 장비는 오른쪽 칸으로 끌어서 끼울 수 있습니다."));
	DetailEffects->SetText(FText::GetEmpty());
	DetailHint->SetText(FText::GetEmpty());
}


void UInventoryWidget::ShowDetailsFor(EInventorySlotKind Kind, int32 Index)
{
	if (!DetailName) return;

	if (Kind == EInventorySlotKind::Card)
	{
		const UMonsterCompanionComponent* Companion = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
		if (!Companion)
		{
			ClearDetails();
			return;
		}
		DetailIcon->SetVisibility(ESlateVisibility::Hidden);
		const int32 NumRegistered = Companion->RegisteredMonsters.Num();
		if (Index < NumRegistered)
		{
			const FRegisteredMonsterEntry& Entry = Companion->RegisteredMonsters[Index];
			const ABaseMonster* Monster = Entry.MonsterClass ? Entry.MonsterClass->GetDefaultObject<ABaseMonster>() : nullptr;
			DetailName->SetText(Monster ? Monster->GetMonsterDisplayName() : FText::GetEmpty());
			DetailName->SetColorAndOpacity(FSlateColor(InventoryStyle::CardGold));
			DetailMeta->SetText(FText::Format(NSLOCTEXT("Inventory", "CardMetaRegistered", "몬스터 카드 · 도감 등록 · Lv.{0} · 강화 +{1}"), Entry.Level, Entry.UpgradeCount));
			DetailDescription->SetText(NSLOCTEXT("Inventory", "CardDescRegistered", "소환할 수 있는 동료입니다. 같은 카드를 또 얻으면 강화되어 능력치가 15%씩 오릅니다."));
			DetailEffects->SetText(FText::FromString(FString::Printf(TEXT("능력치 ×%.2f   경험치 %d / %d"), Entry.GetUpgradeMultiplier(), Entry.CurrentEXP, Entry.GetEXPToNextLevel())));
			DetailHint->SetText(NSLOCTEXT("Inventory", "CardHintRegistered", "Y로 소환"));
		}
		else if (Companion->HeldCards.IsValidIndex(Index - NumRegistered))
		{
			const FMonsterCardEntry& Card = Companion->HeldCards[Index - NumRegistered];
			const ABaseMonster* Monster = Card.MonsterClass ? Card.MonsterClass->GetDefaultObject<ABaseMonster>() : nullptr;
			DetailName->SetText(Monster ? Monster->GetMonsterDisplayName() : FText::GetEmpty());
			DetailName->SetColorAndOpacity(FSlateColor(InventoryStyle::TextMain));
			DetailMeta->SetText(FText::Format(NSLOCTEXT("Inventory", "CardMetaHeld", "몬스터 카드 · 미등록 · 얻은 레벨 {0}"), Card.CapturedLevel));
			DetailDescription->SetText(NSLOCTEXT("Inventory", "CardDescHeld", "도감에 등록하면 소환할 수 있습니다. 이미 있는 종족이면 그 소환수가 강화됩니다."));
			DetailEffects->SetText(FText::GetEmpty());
			DetailHint->SetText(NSLOCTEXT("Inventory", "CardHintHeld", "우클릭: 도감에 등록"));
		}
		else
		{
			ClearDetails();
		}
		return;
	}

	int32 Count = 0;
	const UItemDefinition* Item = GetSlotItem(Kind, Index, Count);
	if (!Item)
	{
		if (Kind == EInventorySlotKind::Equipment)
		{
			ClearDetails();
			DetailName->SetText(FText::Format(NSLOCTEXT("Inventory", "EmptyEquip", "{0} 칸 (비어 있음)"),
				UItemDefinition::GetEquipSlotName(UInventoryComponent::GetSlotTypeForEquipIndex(Index))));
			DetailDescription->SetText(NSLOCTEXT("Inventory", "EmptyEquipHelp", "이 부위 장비를 끌어다 놓거나, 인벤토리에서 우클릭하면 낍니다."));
		}
		else
		{
			ClearDetails();
		}
		return;
	}

	if (Item->Icon)
	{
		DetailIcon->SetBrushFromTexture(Item->Icon);
		DetailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		DetailIcon->SetVisibility(ESlateVisibility::Hidden);
	}

	DetailName->SetText(Count > 1 ? FText::Format(NSLOCTEXT("Inventory", "NameWithCount", "{0}  ×{1}"), Item->DisplayName, Count) : Item->DisplayName);
	DetailName->SetColorAndOpacity(FSlateColor(UItemDefinition::GetRarityColor(Item->Rarity)));

	FText Meta = FText::Format(NSLOCTEXT("Inventory", "Meta", "{0} · {1}"), UItemDefinition::GetCategoryName(Item->Category), UItemDefinition::GetRarityName(Item->Rarity));
	if (Item->IsEquipment())
	{
		Meta = FText::Format(NSLOCTEXT("Inventory", "MetaEquip", "{0} · {1}"), Meta, UItemDefinition::GetEquipSlotName(Item->EquipSlot));
	}
	DetailMeta->SetText(Meta);
	DetailDescription->SetText(Item->Description);

	// 효과 줄. 인벤토리의 장비면 지금 그 부위에 낀 장비도 함께 보여줌
	TArray<FText> Lines = Item->GetEffectLines();
	FString EffectString = FText::Join(FText::FromString(TEXT("   ")), Lines).ToString();
	if (Kind == EInventorySlotKind::Inventory && Item->IsEquipment())
	{
		if (const UInventoryComponent* Inventory = GetInventory())
		{
			for (int32 EquipIndex = 0; EquipIndex < UInventoryComponent::NumEquipSlots; ++EquipIndex)
			{
				if (UInventoryComponent::CanEquipAt(Item, EquipIndex) && Inventory->Equipped.IsValidIndex(EquipIndex) && Inventory->Equipped[EquipIndex])
				{
					EffectString += FString::Printf(TEXT("\n지금 장착: %s"), *Inventory->Equipped[EquipIndex]->DisplayName.ToString());
					break;
				}
			}
		}
	}
	DetailEffects->SetText(FText::FromString(EffectString));

	FText Hint;
	if (Kind == EInventorySlotKind::Equipment)
	{
		Hint = NSLOCTEXT("Inventory", "HintUnequip", "우클릭: 해제");
	}
	else if (Item->IsEquipment())
	{
		Hint = NSLOCTEXT("Inventory", "HintEquip", "우클릭: 장착");
	}
	else if (Item->IsConsumable())
	{
		Hint = NSLOCTEXT("Inventory", "HintUse", "우클릭: 사용");
	}
	else
	{
		Hint = NSLOCTEXT("Inventory", "HintMaterial", "재료 · 지금은 모아 두기만 합니다");
	}
	DetailHint->SetText(Hint);
}


UItemDefinition* UInventoryWidget::GetSlotItem(EInventorySlotKind Kind, int32 Index, int32& OutCount) const
{
	OutCount = 0;
	const UInventoryComponent* Inventory = GetInventory();
	if (!Inventory) return nullptr;

	if (Kind == EInventorySlotKind::Inventory && Inventory->Slots.IsValidIndex(Index) && !Inventory->Slots[Index].IsEmpty())
	{
		OutCount = Inventory->Slots[Index].Count;
		return Inventory->Slots[Index].Item;
	}
	if (Kind == EInventorySlotKind::Equipment && Inventory->Equipped.IsValidIndex(Index) && Inventory->Equipped[Index])
	{
		OutCount = 1;
		return Inventory->Equipped[Index];
	}
	return nullptr;
}


int32 UInventoryWidget::GetCardCount() const
{
	const UMonsterCompanionComponent* Companion = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
	return Companion ? Companion->RegisteredMonsters.Num() + Companion->HeldCards.Num() : 0;
}


void UInventoryWidget::ShowMessage(const FText& Message, bool bWarning)
{
	if (!MessageText) return;
	MessageText->SetText(Message);
	MessageText->SetColorAndOpacity(FSlateColor(bWarning ? InventoryStyle::Warning : InventoryStyle::Accent));
	MessageTimeLeft = 2.5f;
}


// ─── 칸 입력 ───────────────────────────────────────

void UInventoryWidget::OnSlotHovered(UInventorySlotWidget* SlotWidget)
{
	HoveredKind = SlotWidget->GetKind();
	HoveredIndex = SlotWidget->GetIndex();
	RefreshDetails();
}


void UInventoryWidget::OnSlotUnhovered(UInventorySlotWidget* SlotWidget)
{
	if (HoveredKind == SlotWidget->GetKind() && HoveredIndex == SlotWidget->GetIndex())
	{
		HoveredKind = EInventorySlotKind::Filler;
		HoveredIndex = INDEX_NONE;
		RefreshDetails();
	}
}


void UInventoryWidget::OnSlotClicked(UInventorySlotWidget* SlotWidget)
{
	SelectSlot(SlotWidget->GetKind(), SlotWidget->GetIndex());
}


void UInventoryWidget::SelectSlot(EInventorySlotKind Kind, int32 Index)
{
	SelectedKind = Kind;
	SelectedIndex = Index;
	for (UInventorySlotWidget* Cell : GridSlots)
	{
		Cell->SetSelected(Cell->GetKind() == Kind && Cell->GetIndex() == Index);
	}
	for (UInventorySlotWidget* Cell : EquipSlots)
	{
		if (Cell)
		{
			Cell->SetSelected(Cell->GetKind() == Kind && Cell->GetIndex() == Index);
		}
	}
	RefreshDetails();
}


void UInventoryWidget::OnSlotActivated(UInventorySlotWidget* SlotWidget)
{
	UInventoryComponent* Inventory = GetInventory();
	if (!Inventory) return;

	const int32 Index = SlotWidget->GetIndex();
	switch (SlotWidget->GetKind())
	{
	case EInventorySlotKind::Inventory:
	{
		int32 Count = 0;
		const UItemDefinition* Item = GetSlotItem(EInventorySlotKind::Inventory, Index, Count);
		if (!Item) return;
		const FText ItemName = Item->DisplayName;
		const bool bEquipment = Item->IsEquipment();
		FText Message;
		const bool bOk = Inventory->UseSlot(Index, Message);
		if (bOk && bEquipment)
		{
			Message = FText::Format(NSLOCTEXT("Inventory", "Equipped", "{0} 장착"), ItemName);
		}
		ShowMessage(Message, !bOk);
		break;
	}
	case EInventorySlotKind::Equipment:
	{
		int32 Count = 0;
		const UItemDefinition* Item = GetSlotItem(EInventorySlotKind::Equipment, Index, Count);
		if (!Item) return;
		const FText ItemName = Item->DisplayName;
		if (Inventory->Unequip(Index))
		{
			ShowMessage(FText::Format(NSLOCTEXT("Inventory", "Unequipped", "{0} 해제"), ItemName));
		}
		else
		{
			ShowMessage(NSLOCTEXT("Inventory", "FullCannotUnequip", "인벤토리가 가득 차서 뺄 수 없습니다"), true);
		}
		break;
	}
	case EInventorySlotKind::Card:
	{
		UMonsterCompanionComponent* Companion = Rabbit.IsValid() ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
		if (!Companion) return;
		const int32 HeldIndex = Index - Companion->RegisteredMonsters.Num();
		if (HeldIndex < 0)
		{
			ShowMessage(NSLOCTEXT("Inventory", "AlreadyRegistered", "이미 도감에 등록된 소환수입니다"));
		}
		else if (Companion->RegisterCard(HeldIndex))
		{
			ShowMessage(NSLOCTEXT("Inventory", "Registered", "도감에 등록했습니다"));
			RefreshGrid();
		}
		break;
	}
	default:
		break;
	}
}


bool UInventoryWidget::HandleDrop(UInventoryDragOperation* Operation, UInventorySlotWidget* Target)
{
	UInventoryComponent* Inventory = GetInventory();
	if (!Inventory || !Operation || !Target) return false;

	const EInventorySlotKind SourceKind = Operation->SourceKind;
	const int32 SourceIndex = Operation->SourceIndex;
	const EInventorySlotKind TargetKind = Target->GetKind();
	const int32 TargetIndex = Target->GetIndex();

	if (SourceKind == EInventorySlotKind::Inventory && TargetKind == EInventorySlotKind::Inventory)
	{
		Inventory->MoveSlot(SourceIndex, TargetIndex);
		return true;
	}
	if (SourceKind == EInventorySlotKind::Inventory && TargetKind == EInventorySlotKind::Equipment)
	{
		int32 Count = 0;
		const UItemDefinition* Item = GetSlotItem(SourceKind, SourceIndex, Count);
		if (!UInventoryComponent::CanEquipAt(Item, TargetIndex))
		{
			ShowMessage(NSLOCTEXT("Inventory", "WrongSlot", "그 칸에는 낄 수 없습니다"), true);
			return true;
		}
		const FText ItemName = Item->DisplayName;
		if (Inventory->EquipFromSlot(SourceIndex, TargetIndex))
		{
			ShowMessage(FText::Format(NSLOCTEXT("Inventory", "Equipped", "{0} 장착"), ItemName));
		}
		return true;
	}
	if (SourceKind == EInventorySlotKind::Equipment && TargetKind == EInventorySlotKind::Inventory)
	{
		if (!Inventory->Unequip(SourceIndex, TargetIndex))
		{
			ShowMessage(NSLOCTEXT("Inventory", "FullCannotUnequip", "인벤토리가 가득 차서 뺄 수 없습니다"), true);
		}
		return true;
	}
	if (SourceKind == EInventorySlotKind::Equipment && TargetKind == EInventorySlotKind::Equipment)
	{
		if (!Inventory->SwapEquipped(SourceIndex, TargetIndex) && SourceIndex != TargetIndex)
		{
			ShowMessage(NSLOCTEXT("Inventory", "WrongSlot", "그 칸에는 낄 수 없습니다"), true);
		}
		return true;
	}
	return false;
}


// ─── 키·마우스·틱 ───────────────────────────────────────

FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::I || Key == EKeys::Tab || Key == EKeys::Escape)
	{
		if (ARabbit* Owner = Rabbit.Get())
		{
			Owner->CloseInventory();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


FReply UInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 캐릭터 모습 위에서 누르면 끌어서 돌리기 시작
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && PreviewImage
		&& PreviewImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bPreviewDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return FReply::Handled();
}


FReply UInventoryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewDragging)
	{
		bPreviewDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}


FReply UInventoryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPreviewDragging)
	{
		PreviewYaw -= InMouseEvent.GetCursorDelta().X * 0.5f;
		UpdatePreviewTransform();
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}


void UInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (MessageTimeLeft > 0.f)
	{
		MessageTimeLeft -= InDeltaTime;
		if (MessageTimeLeft <= 0.f && MessageText)
		{
			MessageText->SetText(FText::GetEmpty());
		}
	}

	// HP·MP는 계속 바뀌므로 0.2초마다 능력치를 다시 씀
	StatRefreshTime -= InDeltaTime;
	if (StatRefreshTime <= 0.f)
	{
		StatRefreshTime = 0.2f;
		RefreshStats();
	}
}


// ─── 토끼 모습 촬영 ───────────────────────────────────────
// 토끼에 촬영용 카메라와 조명을 붙여 토끼만 찍음(다른 물체·하늘·안개는 안 찍힘). 조명은 조명 채널 1만 써서 토끼에만 비춤

void UInventoryWidget::StartPreview()
{
	ARabbit* Owner = Rabbit.Get();
	USkeletalMeshComponent* Mesh = Owner ? Owner->GetMesh() : nullptr;
	if (!Owner || !Mesh || !PreviewImage) return;

	if (!PreviewTarget)
	{
		PreviewTarget = NewObject<UTextureRenderTarget2D>(this);
		PreviewTarget->ClearColor = FLinearColor::Black;
		PreviewTarget->InitCustomFormat(InventoryLayout::PreviewTextureWidth, InventoryLayout::PreviewTextureHeight, PF_B8G8R8A8, false);
		PreviewTarget->UpdateResourceImmediate(true);
	}
	PreviewImage->SetBrushResourceObject(PreviewTarget);

	if (!PreviewCapture)
	{
		PreviewCapture = NewObject<USceneCaptureComponent2D>(Owner, TEXT("InventoryPreviewCapture"));
		PreviewCapture->SetupAttachment(Owner->GetRootComponent());
		PreviewCapture->RegisterComponent();
		PreviewCapture->TextureTarget = PreviewTarget;
		PreviewCapture->FOVAngle = InventoryLayout::PreviewFOV;
		PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		PreviewCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		PreviewCapture->bCaptureOnMovement = false;
		PreviewCapture->bAlwaysPersistRenderingState = true;
		PreviewCapture->ShowFlags.SetFog(false);
		PreviewCapture->ShowFlags.SetVolumetricFog(false);
		PreviewCapture->ShowFlags.SetAtmosphere(false);
		PreviewCapture->ShowFlags.SetMotionBlur(false);
		PreviewCapture->ShowFlags.SetDepthOfField(false);
		PreviewCapture->ShowFlags.SetLensFlares(false);

		// 노출 고정: 배경이 검은색이라 자동 노출이면 토끼가 하얗게 날아감
		FPostProcessSettings& PP = PreviewCapture->PostProcessSettings;
		PP.bOverride_AutoExposureMethod = true;
		PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		PP.AutoExposureApplyPhysicalCameraExposure = false;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = InventoryLayout::PreviewExposureBias;
		PP.bOverride_MotionBlurAmount = true;
		PP.MotionBlurAmount = 0.f;
		PreviewCapture->PostProcessBlendWeight = 1.f;

		PreviewLight = NewObject<UPointLightComponent>(Owner, TEXT("InventoryPreviewLight"));
		PreviewLight->SetupAttachment(Owner->GetRootComponent());
		PreviewLight->RegisterComponent();
		PreviewLight->SetIntensityUnits(ELightUnits::Candelas);
		PreviewLight->SetIntensity(InventoryLayout::PreviewLightCandelas);
		PreviewLight->SetAttenuationRadius(800.f);
		PreviewLight->SetCastShadows(false);
		PreviewLight->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
		PreviewLight->SetLightingChannels(false, true, false);
	}
	PreviewCapture->ShowOnlyActors.Reset();
	PreviewCapture->ShowOnlyActors.Add(Owner);
	PreviewCapture->bCaptureEveryFrame = true;
	PreviewLight->SetVisibility(true);

	// 토끼 메시 크기에 맞춰 카메라 거리와 바라볼 높이를 정함 (전신이 화면 세로의 약 85%)
	const FBoxSphereBounds Bounds = Mesh->Bounds;
	const FVector LocalCenter = Owner->GetActorTransform().InverseTransformPosition(Bounds.Origin);
	const float HalfHeight = FMath::Max(Bounds.BoxExtent.Z, 20.f);
	const float HalfVerticalFOV = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(InventoryLayout::PreviewFOV * 0.5f))
		* InventoryLayout::PreviewTextureHeight / InventoryLayout::PreviewTextureWidth);
	PreviewLookHeight = LocalCenter.Z;
	PreviewDistance = HalfHeight * 1.18f / FMath::Tan(HalfVerticalFOV);
	PreviewYaw = 25.f;

	// 토끼에게 조명 채널 1을 잠깐 켜서 미리보기 조명이 토끼에만 비치게 함
	SavedLightingChannels.Reset();
	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		SavedLightingChannels.Add(Primitive->LightingChannels);
		Primitive->SetLightingChannels(Primitive->LightingChannels.bChannel0, true, Primitive->LightingChannels.bChannel2);
	}
	UpdatePreviewTransform();
}


void UInventoryWidget::StopPreview()
{
	if (PreviewCapture)
	{
		PreviewCapture->bCaptureEveryFrame = false;
	}
	if (PreviewLight)
	{
		PreviewLight->SetVisibility(false);
	}

	if (ARabbit* Owner = Rabbit.Get())
	{
		TArray<UPrimitiveComponent*> Primitives;
		Owner->GetComponents<UPrimitiveComponent>(Primitives);
		for (int32 Index = 0; Index < Primitives.Num() && Index < SavedLightingChannels.Num(); ++Index)
		{
			const FLightingChannels& Saved = SavedLightingChannels[Index];
			Primitives[Index]->SetLightingChannels(Saved.bChannel0, Saved.bChannel1, Saved.bChannel2);
		}
	}
	SavedLightingChannels.Reset();
}


void UInventoryWidget::UpdatePreviewTransform()
{
	if (!PreviewCapture) return;

	const float YawRadians = FMath::DegreesToRadians(PreviewYaw);
	const FVector CameraOffset(PreviewDistance * FMath::Cos(YawRadians), PreviewDistance * FMath::Sin(YawRadians), PreviewLookHeight + PreviewDistance * 0.06f);
	const FVector LookAt(0.f, 0.f, PreviewLookHeight);
	PreviewCapture->SetRelativeLocationAndRotation(CameraOffset, (LookAt - CameraOffset).Rotation());

	// 조명은 카메라 쪽 위·옆에서 (얼굴에 입체감)
	if (PreviewLight)
	{
		const float LightYaw = YawRadians + FMath::DegreesToRadians(35.f);
		PreviewLight->SetRelativeLocation(FVector(PreviewDistance * 0.7f * FMath::Cos(LightYaw), PreviewDistance * 0.7f * FMath::Sin(LightYaw),
			PreviewLookHeight + PreviewDistance * 0.45f));
	}
}
