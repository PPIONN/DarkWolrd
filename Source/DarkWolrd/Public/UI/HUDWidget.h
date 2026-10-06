// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UCharacterStatComponent;
class UInteractionComponent;

// UHUDWidget: 플레이어 HP/MP를 화면에 표시하는 HUD 위젯의 C++ 베이스.
// WBP_HUD 등 실제 위젯 블루프린트에서 이 클래스를 부모로 지정하고,
// 아래 이름과 정확히 같은 이름의 ProgressBar/TextBlock을 배치하면 자동으로 연결된다(BindWidgetOptional).
UCLASS()
class DARKWOLRD_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// SetStatComponent: HUD가 값을 읽어올 스탯 컴포넌트를 지정. 보통 Rabbit이 위젯을 만든 직후 한 번만 호출
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetStatComponent(UCharacterStatComponent* InStatComponent);

	// SetInteractionComponent: "F: 대화하기" 프롬프트 표시에 쓸 상호작용 컴포넌트를 지정
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetInteractionComponent(UInteractionComponent* InInteractionComponent);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// HPBar / MPBar: 위젯 블루프린트에 이 이름 그대로 ProgressBar를 배치하면 자동 연결됨 (없어도 에러 없음)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UProgressBar* HPBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UProgressBar* MPBar;

	// HPText / MPText: "72 / 120" 형태의 숫자 표기가 필요하면 이 이름으로 TextBlock을 배치
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* HPText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* MPText;

	// InteractPromptText: 상호작용 가능한 NPC를 조준+범위 안에 두면 표시되는 프롬프트 텍스트("F: 대화하기" 등)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* InteractPromptText;

private:
	// BoundStatComponent: 값을 읽어올 대상. 약한 참조로 들고 있어 대상이 파괴돼도 안전함
	UPROPERTY()
	TWeakObjectPtr<UCharacterStatComponent> BoundStatComponent;

	// BoundInteractionComponent: 상호작용 가능 여부를 읽어올 대상
	UPROPERTY()
	TWeakObjectPtr<UInteractionComponent> BoundInteractionComponent;
};
