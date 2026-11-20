// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/InventorySlotWidget.h"
#include "InventoryWidget.generated.h"

class ARabbit;
class UInventoryComponent;
class UItemDefinition;
class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class USceneCaptureComponent2D;
class UPointLightComponent;
class UTextureRenderTarget2D;

// UInventoryWidget: 인벤토리 화면 (I 또는 Tab). 코드로 그림.
// 왼쪽: 탭(전체·장비·소모품·재료·카드), 7×6 아이템 칸, 아래에 고른 아이템 설명.
// 오른쪽: 토끼 모습(실시간으로 찍음, 끌어서 돌림)과 장비 6칸(왼쪽 머리·몸·신발, 오른쪽 무기·장신구 둘), 그 아래 능력치와 돈.
// 좌클릭 선택, 우클릭·더블클릭 사용/장착/해제, 끌어서 옮기기·장착. I·Tab·Esc로 닫음
UCLASS()
class DARKWOLRD_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// OpenFor / CloseInventory: 열 때 토끼에 연결하고 모습 촬영을 시작, 닫을 때 촬영을 멈춤
	void OpenFor(ARabbit* InRabbit);
	void CloseInventory();

	// bPauseWhileOpen: 켜면 여는 동안 게임을 멈춤. 기본은 팰월드처럼 멈추지 않음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bPauseWhileOpen = false;

	// 칸에서 넘어오는 입력
	void OnSlotHovered(UInventorySlotWidget* SlotWidget);
	void OnSlotUnhovered(UInventorySlotWidget* SlotWidget);
	void OnSlotClicked(UInventorySlotWidget* SlotWidget);
	void OnSlotActivated(UInventorySlotWidget* SlotWidget);
	bool HandleDrop(UInventoryDragOperation* Operation, UInventorySlotWidget* Target);

	// SelectSlot: 칸을 선택해 설명을 띄움 (클릭과 같음. 자동 점검 봇도 씀)
	void SelectSlot(EInventorySlotKind Kind, int32 Index);

	// GetSlotItem: 그 칸의 아이템과 개수 (인벤토리·장비 칸만)
	UItemDefinition* GetSlotItem(EInventorySlotKind Kind, int32 Index, int32& OutCount) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeDestruct() override;

	UFUNCTION() void HandleInventoryChanged();
	UFUNCTION() void OnTabAll();
	UFUNCTION() void OnTabEquipment();
	UFUNCTION() void OnTabConsumable();
	UFUNCTION() void OnTabMaterial();
	UFUNCTION() void OnTabCard();
	UFUNCTION() void OnSortClicked();

private:
	enum class ETab : uint8 { All, Equipment, Consumable, Material, Card, Count };

	// 화면 만들기 도우미
	UTextBlock* MakeText(float Size, const FLinearColor& Color, const FText& Text = FText::GetEmpty());
	UBorder* MakePanel(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, const FMargin& InPadding);
	UButton* MakeTabButton(const FText& Label, UTextBlock*& OutLabel);
	UInventorySlotWidget* MakeSlot(EInventorySlotKind Kind, int32 Index, float Size);
	void AddStatRow(UVerticalBox* Box, const FText& Label, UTextBlock*& OutValue, UTextBlock*& OutBonus);

	void SetTab(ETab NewTab);
	void RefreshAll();
	void RefreshGrid();
	void RefreshEquipment();
	void RefreshStats();
	void RefreshDetails();
	void ShowMessage(const FText& Message, bool bWarning = false);

	// 설명 패널에 보여줄 칸 (마우스를 올린 칸 > 선택한 칸)
	void ShowDetailsFor(EInventorySlotKind Kind, int32 Index);
	void ClearDetails();

	// 카드 탭: 도감에 등록된 소환수 다음에 아직 등록 안 한 카드
	int32 GetCardCount() const;

	// 토끼 모습 촬영
	void StartPreview();
	void StopPreview();
	void UpdatePreviewTransform();

	UInventoryComponent* GetInventory() const;

	TWeakObjectPtr<ARabbit> Rabbit;
	ETab CurrentTab = ETab::All;

	EInventorySlotKind SelectedKind = EInventorySlotKind::Filler;
	int32 SelectedIndex = INDEX_NONE;
	EInventorySlotKind HoveredKind = EInventorySlotKind::Filler;
	int32 HoveredIndex = INDEX_NONE;

	float MessageTimeLeft = 0.f;
	float StatRefreshTime = 0.f;

	bool bPreviewDragging = false;
	float PreviewYaw = 0.f;
	float PreviewDistance = 300.f;
	float PreviewLookHeight = 0.f;
	TArray<FLightingChannels> SavedLightingChannels;
	bool bPausedGame = false;

	UPROPERTY() TArray<UInventorySlotWidget*> GridSlots;
	UPROPERTY() TArray<UInventorySlotWidget*> EquipSlots;
	UPROPERTY() TArray<UButton*> TabButtons;
	UPROPERTY() TArray<UTextBlock*> TabLabels;

	UPROPERTY() UTextBlock* CapacityText = nullptr;
	UPROPERTY() UTextBlock* CharacterTitleText = nullptr;
	UPROPERTY() UImage* PreviewImage = nullptr;

	UPROPERTY() UImage* DetailIcon = nullptr;
	UPROPERTY() UTextBlock* DetailName = nullptr;
	UPROPERTY() UTextBlock* DetailMeta = nullptr;
	UPROPERTY() UTextBlock* DetailDescription = nullptr;
	UPROPERTY() UTextBlock* DetailEffects = nullptr;
	UPROPERTY() UTextBlock* DetailHint = nullptr;
	UPROPERTY() UTextBlock* MessageText = nullptr;

	// 능력치 줄: 레벨, HP, MP, 마력, 지능, 체력, 방어력, 공격속도, 이동속도, 돈
	UPROPERTY() TArray<UTextBlock*> StatValues;
	UPROPERTY() TArray<UTextBlock*> StatBonuses;

	UPROPERTY() USceneCaptureComponent2D* PreviewCapture = nullptr;
	UPROPERTY() UPointLightComponent* PreviewLight = nullptr;
	UPROPERTY() UTextureRenderTarget2D* PreviewTarget = nullptr;
};
