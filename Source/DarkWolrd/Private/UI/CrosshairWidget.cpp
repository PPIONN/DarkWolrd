// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CrosshairWidget.h"
#include "Character/Rabbit.h"
#include "Combat/AbilityComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

// 조준점(상하좌우 선 4개 + 가운데 점)과 락온 표시(네 모서리 꺾쇠)를 코드로 만듦
TSharedRef<SWidget> UCrosshairWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// 조준점: 화면 중앙 기준
		const FVector2D ScreenCenter(0.5f, 0.5f);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			UCanvasPanelSlot* BarSlot = nullptr;
			UCanvasPanelSlot* OutlineSlot = nullptr;
			AddBar(Root, ScreenCenter, CrosshairColor, BarSlot, OutlineSlot);
			LineSlots.Add(BarSlot);
			LineOutlineSlots.Add(OutlineSlot);
		}
		CurrentGapValue = Gap;
		UpdateCrosshairLines(CurrentGapValue);

		// 락온 표시: 작은 캔버스 안에 네 모서리 꺾쇠 (모서리마다 가로·세로 막대 하나씩)
		LockOnMarker = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LockOnMarker"));
		UCanvasPanelSlot* MarkerSlot = Root->AddChildToCanvas(LockOnMarker);
		MarkerSlot->SetAnchors(FAnchors(0.f, 0.f));
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		MarkerSlot->SetSize(FVector2D(LockOnMarkerSize, LockOnMarkerSize));

		const float Half = LockOnMarkerSize * 0.5f;
		const float Arm = LockOnMarkerSize * 0.3f;
		const float Thick = 3.f;
		const FVector2D Corners[] = { FVector2D(-1.f, -1.f), FVector2D(1.f, -1.f), FVector2D(-1.f, 1.f), FVector2D(1.f, 1.f) };
		for (const FVector2D& Corner : Corners)
		{
			const FVector2D CornerPoint = Corner * Half;
			UCanvasPanelSlot* BarSlot = nullptr;
			UCanvasPanelSlot* OutlineSlot = nullptr;

			// 가로 막대: 모서리에서 안쪽으로
			AddBar(LockOnMarker, FVector2D(0.5f, 0.5f), LockOnColor, BarSlot, OutlineSlot);
			SetBarRect(BarSlot, OutlineSlot, CornerPoint + FVector2D(-Corner.X * Arm * 0.5f, 0.f), FVector2D(Arm, Thick));

			// 세로 막대: 모서리에서 안쪽으로
			AddBar(LockOnMarker, FVector2D(0.5f, 0.5f), LockOnColor, BarSlot, OutlineSlot);
			SetBarRect(BarSlot, OutlineSlot, CornerPoint + FVector2D(0.f, -Corner.Y * Arm * 0.5f), FVector2D(Thick, Arm));
		}
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
	}

	return Super::RebuildWidget();
}


void UCrosshairWidget::AddBar(UCanvasPanel* Parent, const FVector2D& Anchor, const FLinearColor& Color,
	UCanvasPanelSlot*& OutBarSlot, UCanvasPanelSlot*& OutOutlineSlot)
{
	// 테두리를 먼저 추가해야 막대 뒤에 그려짐
	UImage* Outline = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Outline->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	OutOutlineSlot = Parent->AddChildToCanvas(Outline);

	UImage* Bar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Bar->SetColorAndOpacity(Color);
	OutBarSlot = Parent->AddChildToCanvas(Bar);

	OutOutlineSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	OutOutlineSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	OutBarSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	OutBarSlot->SetAlignment(FVector2D(0.5f, 0.5f));
}


void UCrosshairWidget::SetBarRect(UCanvasPanelSlot* BarSlot, UCanvasPanelSlot* OutlineSlot, const FVector2D& Center, const FVector2D& Size)
{
	BarSlot->SetPosition(Center);
	BarSlot->SetSize(Size);
	OutlineSlot->SetPosition(Center);
	OutlineSlot->SetSize(Size + FVector2D(2.f, 2.f));
}


// 선 순서: 위, 아래, 왼쪽, 오른쪽, 가운데 점
void UCrosshairWidget::UpdateCrosshairLines(float CurrentGap)
{
	if (LineSlots.Num() < 5) return;

	const float Offset = CurrentGap + LineLength * 0.5f;
	const FVector2D Vertical(LineThickness, LineLength);
	const FVector2D Horizontal(LineLength, LineThickness);

	SetBarRect(LineSlots[0], LineOutlineSlots[0], FVector2D(0.f, -Offset), Vertical);
	SetBarRect(LineSlots[1], LineOutlineSlots[1], FVector2D(0.f, Offset), Vertical);
	SetBarRect(LineSlots[2], LineOutlineSlots[2], FVector2D(-Offset, 0.f), Horizontal);
	SetBarRect(LineSlots[3], LineOutlineSlots[3], FVector2D(Offset, 0.f), Horizontal);
	SetBarRect(LineSlots[4], LineOutlineSlots[4], FVector2D::ZeroVector, FVector2D(LineThickness, LineThickness));
}


// 매 프레임: 조준 중이면 간격을 좁히고, 락온 표시를 대상 위치로 옮김
void UCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ARabbit* Rabbit = Cast<ARabbit>(GetOwningPlayerPawn());
	const float TargetGap = (Rabbit && Rabbit->IsAiming()) ? AimGap : Gap;
	if (!FMath::IsNearlyEqual(CurrentGapValue, TargetGap, 0.01f))
	{
		CurrentGapValue = FMath::FInterpTo(CurrentGapValue, TargetGap, InDeltaTime, 15.f);
		UpdateCrosshairLines(CurrentGapValue);
	}

	UpdateLockOnMarker();
}


void UCrosshairWidget::UpdateLockOnMarker()
{
	if (!LockOnMarker) return;

	const APawn* OwnerPawn = GetOwningPlayerPawn();
	const UAbilityComponent* Ability = OwnerPawn ? OwnerPawn->FindComponentByClass<UAbilityComponent>() : nullptr;

	FVector2D ScreenPosition;
	const bool bShow = Ability && Ability->IsLockedOn()
		&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(GetOwningPlayer(), Ability->LockedTarget->GetActorLocation(), ScreenPosition, false);

	if (!bShow)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	LockOnMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(LockOnMarker->Slot))
	{
		MarkerSlot->SetPosition(ScreenPosition);
	}
}
