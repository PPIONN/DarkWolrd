// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialGuideWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class UWidget;
class ARabbit;
class UMonsterCompanionComponent;

// ETutorialStep: 튜토리얼 맵을 처음 하는 사람에게 보여주는 목표 순서
UENUM(BlueprintType)
enum class ETutorialStep : uint8
{
	MoveToJump,
	MoveToAim,
	FirstShot,
	HuntFirst,
	PickUpCard,
	Summon,
	HuntTogether,
	GoToExit,
	Done
};

// UTutorialGuideWidget: 조작 안내, 지금 할 일(목표), 목표 지점 표시, 화면 알림을 보여주는 위젯.
// 설치본만 받은 사람도 혼자 끝까지 해볼 수 있게 하는 용도이며, 위젯 BP 없이 코드로 그려짐.
// 목표 안내는 Tutorial 맵에서만 나오고, 다른 맵에서는 조작 안내와 화면 알림만 나옴
UCLASS()
class DARKWOLRD_API UTutorialGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// HuntTogetherKills: 소환수와 함께 잡아야 하는 박쥐 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guide")
	int32 HuntTogetherKills = 3;

	UFUNCTION(BlueprintCallable, Category = "Guide")
	ETutorialStep GetCurrentStep() const { return CurrentStep; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// HandleAnnouncement: UGameAnnouncementSubsystem에서 알림이 오면 화면 위쪽에 표시
	UFUNCTION()
	void HandleAnnouncement(const FText& Message, float Duration);

	// MakeText: 어두운 맵과 밝은 하늘 모두에서 읽히도록 외곽선이 있는 글자를 만듦
	UTextBlock* MakeText(float FontSize, const FLinearColor& Color);

	// UpdateStep: 현재 목표의 달성 여부를 보고 다음 목표로 넘어감. 순서를 건너뛰어 행동해도 맞는 목표로 따라감
	void UpdateStep();

	// EnterStep: 목표를 바꾸고 달성 소리를 재생
	void EnterStep(ETutorialStep NewStep);

	// RefreshObjectiveText: 목표 문구를 지금 상태에 맞게 갱신
	void RefreshObjectiveText();

	// UpdateMarker: 목표 지점 표시를 옮김. 화면 밖이면 그 방향의 화면 가장자리에 붙임
	void UpdateMarker();

	// GetObjectiveLocation: 현재 목표 지점의 월드 위치. 지점이 없는 목표면 false
	bool GetObjectiveLocation(FVector& OutLocation) const;

	// FindDroppedCard: 플레이어와 가장 가까운 바닥의 카드
	AActor* FindDroppedCard() const;

	ARabbit* GetRabbit() const;
	UMonsterCompanionComponent* GetCompanion() const;

	UPROPERTY()
	UWidget* ObjectivePanel = nullptr;

	UPROPERTY()
	UTextBlock* StepCountText = nullptr;

	UPROPERTY()
	UTextBlock* ObjectiveText = nullptr;

	UPROPERTY()
	UTextBlock* AnnouncementText = nullptr;

	UPROPERTY()
	UCanvasPanel* Marker = nullptr;

	UPROPERTY()
	UTextBlock* MarkerDistanceText = nullptr;

	ETutorialStep CurrentStep = ETutorialStep::MoveToJump;
	bool bShowObjectives = false;

	// 지금 목표를 시작할 때의 공격 횟수·처치 수 (이 목표에서 새로 한 만큼만 셈)
	int32 StepStartAttackCount = 0;
	int32 StepStartKillCount = 0;

	// AnnouncementHideTime: 현재 알림을 숨길 시각 (월드 시간)
	float AnnouncementHideTime = 0.f;
};
