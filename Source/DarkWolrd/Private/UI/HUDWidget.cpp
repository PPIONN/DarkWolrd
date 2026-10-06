// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Stats/CharacterStatComponent.h"
#include "Interaction/InteractionComponent.h"
#include "NPC/NPCBase.h"

void UHUDWidget::SetStatComponent(UCharacterStatComponent* InStatComponent)
{
	BoundStatComponent = InStatComponent;
}


void UHUDWidget::SetInteractionComponent(UInteractionComponent* InInteractionComponent)
{
	BoundInteractionComponent = InInteractionComponent;
}


// 매 프레임 HP/MP를 다시 읽어 바/텍스트를 갱신. 스탯 값이 바뀌는 지점(회복 타이머, 명상, 레벨업, 피격 등)이
// 여러 곳에 흩어져 있어 델리게이트 대신 단순 폴링으로 처리 - 값 두 개 갱신이라 비용도 무시할 수준
void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UCharacterStatComponent* Stat = BoundStatComponent.Get();
	if (!Stat) return;

	const float MaxHP = FMath::Max(Stat->GetMaxHP(), 1.f);
	const float MaxMP = FMath::Max(Stat->GetMaxMP(), 1.f);

	if (HPBar)
	{
		HPBar->SetPercent(Stat->CurrentHP / MaxHP);
	}
	if (MPBar)
	{
		MPBar->SetPercent(Stat->CurrentMP / MaxMP);
	}
	if (HPText)
	{
		HPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Stat->CurrentHP), FMath::RoundToInt(MaxHP))));
	}
	if (MPText)
	{
		MPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Stat->CurrentMP), FMath::RoundToInt(MaxMP))));
	}

	if (InteractPromptText)
	{
		UInteractionComponent* Interaction = BoundInteractionComponent.Get();
		const bool bCanInteract = Interaction && Interaction->CurrentInteractableNPC;

		InteractPromptText->SetVisibility(bCanInteract ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bCanInteract)
		{
			InteractPromptText->SetText(FText::Format(NSLOCTEXT("HUD", "InteractPrompt", "F: {0}와 대화하기"), Interaction->CurrentInteractableNPC->NPCDisplayName));
		}
	}
}
