// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Stats/CharacterStatComponent.h"
#include "Interaction/InteractionComponent.h"
#include "NPC/NPCBase.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Monster/FieldBossMonster.h"
#include "EngineUtils.h"

// 생성: 알림 텍스트가 배치돼 있을 때만 알림을 구독 (없으면 서브시스템이 디버그 글씨로 대신 표시하도록 비워둠)
void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AnnouncementText)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
		if (UGameAnnouncementSubsystem* Announcements = GetWorld() ? GetWorld()->GetSubsystem<UGameAnnouncementSubsystem>() : nullptr)
		{
			Announcements->OnAnnouncement.AddDynamic(this, &UHUDWidget::HandleAnnouncement);
		}
	}
	if (BossPanel)
	{
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}


void UHUDWidget::NativeDestruct()
{
	if (UGameAnnouncementSubsystem* Announcements = GetWorld() ? GetWorld()->GetSubsystem<UGameAnnouncementSubsystem>() : nullptr)
	{
		Announcements->OnAnnouncement.RemoveDynamic(this, &UHUDWidget::HandleAnnouncement);
	}
	Super::NativeDestruct();
}


void UHUDWidget::HandleAnnouncement(const FText& Message, float Duration)
{
	if (!AnnouncementText) return;

	AnnouncementText->SetText(Message);
	AnnouncementText->SetVisibility(ESlateVisibility::HitTestInvisible);
	AnnouncementHideTime = GetWorld()->GetTimeSeconds() + Duration;
}


// 보스 HP바: 보스 수는 많지 않아서 매 프레임 월드의 필드보스를 훑어도 부담이 없음
void UHUDWidget::UpdateBossPanel()
{
	if (!BossPanel) return;

	const APawn* PlayerPawn = GetOwningPlayerPawn();
	const AFieldBossMonster* NearestBoss = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	if (PlayerPawn)
	{
		for (TActorIterator<AFieldBossMonster> It(GetWorld()); It; ++It)
		{
			const AFieldBossMonster* Boss = *It;
			if (!Boss->IsAlive() || Boss->bIsPlayerSummon) continue;

			const float DistSq = FVector::DistSquared(Boss->GetActorLocation(), PlayerPawn->GetActorLocation());
			if (DistSq <= FMath::Square(Boss->HUDShowDistance) && DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestBoss = Boss;
			}
		}
	}

	if (!NearestBoss)
	{
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	BossPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (BossNameText)
	{
		BossNameText->SetText(NearestBoss->GetBossDisplayName());
	}
	if (BossHPBar)
	{
		BossHPBar->SetPercent(NearestBoss->CurrentHP / FMath::Max(NearestBoss->GetMaxHP(), 1.f));
	}
}


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

	if (AnnouncementText && AnnouncementText->IsVisible() && GetWorld()->GetTimeSeconds() >= AnnouncementHideTime)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
	}
	UpdateBossPanel();

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
