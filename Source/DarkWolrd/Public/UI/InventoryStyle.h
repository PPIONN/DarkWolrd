// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

// 인벤토리 화면 색과 둥근 사각형 브러시. 짙은 남색 반투명 판 위에 밝은 글씨, 희귀도 색 테두리.
// UMG 색은 선형값이라 화면에서는 더 밝게 보임(0.03 ≈ sRGB 48). 처음 값은 회색빛으로 떠서 한 번 어둡게 낮췄음
namespace InventoryStyle
{
	inline const FLinearColor PanelFill(0.012f, 0.015f, 0.028f, 0.94f);
	inline const FLinearColor PanelOutline(0.25f, 0.32f, 0.55f, 0.35f);
	inline const FLinearColor SubPanelFill(0.02f, 0.025f, 0.042f, 0.95f);
	inline const FLinearColor SlotFill(0.028f, 0.034f, 0.055f, 1.f);
	inline const FLinearColor SlotFillHover(0.06f, 0.072f, 0.11f, 1.f);
	inline const FLinearColor SlotOutline(0.1f, 0.12f, 0.18f, 0.9f);
	inline const FLinearColor TabActiveFill(0.06f, 0.045f, 0.02f, 1.f);
	inline const FLinearColor Accent(1.f, 0.6f, 0.15f, 1.f);
	inline const FLinearColor TextMain(0.93f, 0.95f, 1.f, 1.f);
	inline const FLinearColor TextDim(0.38f, 0.42f, 0.52f, 1.f);
	inline const FLinearColor TextFaint(0.2f, 0.23f, 0.3f, 1.f);
	inline const FLinearColor Bonus(0.3f, 0.9f, 0.4f, 1.f);
	inline const FLinearColor Warning(1.f, 0.35f, 0.25f, 1.f);
	inline const FLinearColor CardGold(1.f, 0.65f, 0.2f, 1.f);

	// MakeRoundedBrush: 채움색 + 테두리 색·두께 + 모서리 반지름
	inline FSlateBrush MakeRoundedBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Radius, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}
}
