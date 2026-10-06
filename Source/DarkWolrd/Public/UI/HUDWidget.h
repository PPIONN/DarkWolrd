// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UWidget;
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ─── 화면 알림 ("필드보스가 나타났습니다!" 등) ───────────────────────────────────────
	// AnnouncementText: 이 이름의 TextBlock을 화면 위쪽 가운데쯤 배치하면 알림이 여기에 표시됨.
	// 배치하지 않으면 알림은 화면 왼쪽 위 디버그 글씨로 대신 나옴
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* AnnouncementText;

	// ─── 필드보스 HP바 (보스 가까이 가면 표시) ───────────────────────────────────────
	// BossPanel: 보스 이름과 HP바를 묶은 패널(Overlay/VerticalBox 등). 보스가 없으면 통째로 숨김
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UWidget* BossPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* BossNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UProgressBar* BossHPBar;

	// HandleAnnouncement: UGameAnnouncementSubsystem에서 알림이 오면 AnnouncementText에 표시
	UFUNCTION()
	void HandleAnnouncement(const FText& Message, float Duration);

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

	// UpdateBossPanel: 플레이어와 가장 가까운(HUDShowDistance 안의) 살아있는 필드보스를 찾아 이름·HP 표시
	void UpdateBossPanel();

	// AnnouncementHideTime: 현재 알림을 숨길 시각 (월드 시간)
	float AnnouncementHideTime = 0.f;
};
