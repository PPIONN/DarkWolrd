// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "InventorySlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class USizeBox;
class UItemDefinition;
class UInventoryWidget;

// EInventorySlotKind: 칸이 무엇을 가리키는지. Filler는 탭으로 걸러서 남는 빈 자리(놓기 불가)
UENUM()
enum class EInventorySlotKind : uint8
{
	Inventory,
	Equipment,
	Card,
	Filler,
};

// UInventoryDragOperation: 칸을 끌 때 어디서 끌었는지 담는 정보
UCLASS()
class DARKWOLRD_API UInventoryDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	EInventorySlotKind SourceKind = EInventorySlotKind::Inventory;
	int32 SourceIndex = INDEX_NONE;
};

// UInventorySlotWidget: 인벤토리·장비·카드 칸 하나. 코드로 그림(둥근 칸 + 희귀도 테두리 + 아이콘 + 개수).
// 마우스 올리면 설명, 좌클릭 선택, 우클릭·더블클릭 사용/장착, 끌어서 옮기기는 전부 UInventoryWidget에 넘김
UCLASS()
class DARKWOLRD_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Setup: 어떤 칸인지 지정 (탭이 바뀌면 다시 부름)
	void Setup(UInventoryWidget* InOwner, EInventorySlotKind InKind, int32 InIndex, float InSize);

	// ShowItem: 아이템 칸 내용. Item이 없으면 빈 칸(EmptyLabel이 있으면 흐리게 표시, 예: 장비 칸 '무기')
	void ShowItem(UItemDefinition* InItem, int32 InCount, const FText& EmptyLabel = FText::GetEmpty());

	// ShowCard: 카드 칸 내용. Badge는 오른쪽 아래 작은 글씨(Lv·강화), bRegistered가 아니면 흐린 테두리
	void ShowCard(const FText& Name, const FText& Badge, bool bRegistered);

	void SetSelected(bool bInSelected);

	EInventorySlotKind GetKind() const { return Kind; }
	int32 GetIndex() const { return Index; }
	bool HasContent() const { return bHasContent; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	void RefreshFrame();

	TWeakObjectPtr<UInventoryWidget> OwnerWidget;
	EInventorySlotKind Kind = EInventorySlotKind::Filler;
	int32 Index = INDEX_NONE;
	float SlotSize = 72.f;

	bool bHasContent = false;
	bool bSelected = false;
	bool bHovered = false;
	bool bDragOver = false;
	FLinearColor RarityColor = FLinearColor::White;

	UPROPERTY() USizeBox* SizeBox = nullptr;
	UPROPERTY() UBorder* Frame = nullptr;
	UPROPERTY() UImage* Icon = nullptr;
	UPROPERTY() UTextBlock* NameText = nullptr;
	UPROPERTY() UTextBlock* CountText = nullptr;
	UPROPERTY() UBorder* RarityBar = nullptr;
};
