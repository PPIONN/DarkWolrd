// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemPickupFeedWidget.generated.h"

class UVerticalBox;
class UInventoryComponent;
class UItemDefinition;
class UTextBlock;
class UBorder;

// UItemPickupFeedWidget: 아이템을 주우면 화면 오른쪽에 "+2 박쥐 날개"를 잠깐 띄움(희귀도 색). 같은 아이템을 연달아 주우면 한 줄에 합침
UCLASS()
class DARKWOLRD_API UItemPickupFeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindInventory(UInventoryComponent* Inventory);

	// LineLifetime: 한 줄이 떠 있는 시간(초). 마지막 0.6초 동안 흐려짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Feed")
	float LineLifetime = 3.5f;

	// MaxLines: 동시에 보이는 최대 줄 수 (넘으면 오래된 줄부터 지움)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Feed")
	int32 MaxLines = 5;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleItemAdded(UItemDefinition* Item, int32 Count);

private:
	struct FLine
	{
		TWeakObjectPtr<UBorder> Row;
		TWeakObjectPtr<UTextBlock> Text;
		TWeakObjectPtr<UItemDefinition> Item;
		int32 Count = 0;
		float TimeLeft = 0.f;
	};
	TArray<FLine> Lines;

	UPROPERTY() UVerticalBox* List = nullptr;
};
