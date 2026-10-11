// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CrosshairWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;

// UCrosshairWidget: 화면 중앙 조준점 + 락온 대상 표시.
// 평타 오브는 화면 중앙(카메라 정면)으로 날아가므로 조준점은 항상 화면 정중앙.
// 조준(우클릭) 중에는 조준점 간격이 좁아지고, 락온 중에는 대상 몸 위에 모서리 꺾쇠 표시가 따라다님.
// 위젯 BP 없이 코드로 그려짐 (밝은 바닥에서도 보이도록 검은 테두리 포함)
UCLASS()
class DARKWOLRD_API UCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// CrosshairColor / LockOnColor: 조준점 색 / 락온 표시 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor CrosshairColor = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor LockOnColor = FLinearColor(1.f, 0.25f, 0.2f, 1.f);

	// Gap / AimGap: 중앙에서 선까지의 간격(픽셀). 조준 중에는 AimGap으로 좁아짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float Gap = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float AimGap = 3.f;

	// LineLength / LineThickness: 조준점 선 길이와 두께(픽셀)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float LineLength = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float LineThickness = 2.f;

	// LockOnMarkerSize: 락온 표시(네 모서리 꺾쇠)의 한 변 크기(픽셀)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float LockOnMarkerSize = 44.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// AddBar: Parent 캔버스에 검은 테두리가 있는 막대를 추가하고, 막대(앞)와 테두리(뒤) 슬롯을 돌려줌
	void AddBar(UCanvasPanel* Parent, const FVector2D& Anchor, const FLinearColor& Color,
		UCanvasPanelSlot*& OutBarSlot, UCanvasPanelSlot*& OutOutlineSlot);

	// SetBarRect: 막대 중심 위치·크기를 정하고, 테두리는 1픽셀씩 더 크게 맞춤
	static void SetBarRect(UCanvasPanelSlot* BarSlot, UCanvasPanelSlot* OutlineSlot, const FVector2D& Center, const FVector2D& Size);

	// UpdateCrosshairLines: 현재 간격으로 상하좌우 선 위치를 다시 잡음
	void UpdateCrosshairLines(float CurrentGap);

	// UpdateLockOnMarker: 락온 대상의 화면 위치로 표시를 옮기거나, 락온이 아니면 숨김
	void UpdateLockOnMarker();

	UPROPERTY()
	TArray<UCanvasPanelSlot*> LineSlots;

	UPROPERTY()
	TArray<UCanvasPanelSlot*> LineOutlineSlots;

	UPROPERTY()
	UCanvasPanel* LockOnMarker = nullptr;

	float CurrentGapValue = 7.f;
};
