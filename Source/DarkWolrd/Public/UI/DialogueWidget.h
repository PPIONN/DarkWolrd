// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

class UTextBlock;

// UDialogueWidget: NPC 대사를 보여주는 대화창 위젯의 C++ 베이스.
// WBP_Dialogue 등 실제 위젯 블루프린트에서 이 클래스를 부모로 지정하고,
// 아래 이름과 정확히 같은 이름의 TextBlock을 배치하면 자동으로 연결된다(BindWidgetOptional)
UCLASS()
class DARKWOLRD_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ShowLine: NPC 이름과 대사 한 줄을 표시
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ShowLine(const FText& NPCName, const FText& Line);

protected:
	// NPCNameText / DialogueLineText: 위젯 블루프린트에 이 이름 그대로 TextBlock을 배치하면 자동 연결됨
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* NPCNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* DialogueLineText;
};
