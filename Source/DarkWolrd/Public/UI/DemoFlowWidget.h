// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DemoFlowWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWidget;
class ARabbit;

// EDemoScreen: 데모 한 판의 흐름에서 지금 떠 있는 화면
UENUM(BlueprintType)
enum class EDemoScreen : uint8
{
	None,		// 플레이 중
	Title,		// 시작 화면
	Failed,		// 쓰러짐
	Cleared		// 출구 도착
};

// UDemoFlowWidget: 데모 v1의 시작 → 플레이 → 끝을 잇는 전체 화면.
// 시작 화면(시작하기·종료), 실패 화면(마지막 목표에서 다시·처음부터·종료), 클리어 화면(계속 사냥·처음부터·종료)을 보여줌.
// 화면이 떠 있는 동안은 게임을 멈추고 마우스로 버튼을 고름. Tutorial 맵에서만 쓰며, 위젯 BP 없이 코드로 그려짐
UCLASS()
class DARKWOLRD_API UDemoFlowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Demo")
	EDemoScreen GetScreen() const { return Screen; }

	// StartGame: 시작 화면을 닫고 플레이 시작
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartGame();

	// RetryFromCheckpoint: 쓰러진 뒤 마지막으로 달성한 목표 위치에서 다시
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void RetryFromCheckpoint();

	// RestartFromBeginning: 맵을 다시 불러와 시작 화면부터
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void RestartFromBeginning();

	// ContinueHunting: 클리어 화면을 닫고 맵에 남아 계속 사냥
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ContinueHunting();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void QuitDemo();

	// TickFlow: 예약된 실패 화면을 띄우고, 튜토리얼 목표를 모두 끝냈으면 클리어 화면을 예약. 토끼의 월드 틱에서 호출
	void TickFlow();

	// ClearDelay: 출구에 닿은 뒤 클리어 화면이 뜨기까지의 시간(초). 목표 달성 소리가 들릴 틈
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float ClearDelay = 1.5f;

	// FailDelay: 쓰러진 뒤 실패 화면이 뜨기까지의 시간(초). 쓰러지는 모습이 보일 틈
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float FailDelay = 1.5f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleRabbitDied();

	// ShowScreen: 화면을 띄우고 게임을 멈춘 뒤 마우스로 고를 수 있게 함. None이면 닫고 다시 플레이
	void ShowScreen(EDemoScreen NewScreen);

	UTextBlock* MakeText(float FontSize, const FLinearColor& Color, UVerticalBox* Parent);
	UButton* MakeButton(const FText& Label, UVerticalBox* Parent);

	ARabbit* GetRabbit() const;

	UPROPERTY()
	UWidget* Overlay = nullptr;

	UPROPERTY()
	UTextBlock* HeadlineText = nullptr;

	UPROPERTY()
	UTextBlock* SubText = nullptr;

	UPROPERTY()
	UTextBlock* BodyText = nullptr;

	UPROPERTY()
	UButton* StartButton = nullptr;

	UPROPERTY()
	UButton* RetryButton = nullptr;

	UPROPERTY()
	UButton* ContinueButton = nullptr;

	UPROPERTY()
	UButton* RestartButton = nullptr;

	UPROPERTY()
	UButton* QuitButton = nullptr;

	EDemoScreen Screen = EDemoScreen::None;

	// 한 판의 기록 (클리어 화면에 표시)
	float PlayStartTime = 0.f;
	int32 DeathCount = 0;
	bool bCleared = false;

	// 화면을 띄울 예정 시각 (월드 시간, 0이면 예정 없음)
	float PendingFailTime = 0.f;
	float PendingClearTime = 0.f;
};
