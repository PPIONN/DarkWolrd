// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DialogueWidget.h"
#include "Components/TextBlock.h"

void UDialogueWidget::ShowLine(const FText& NPCName, const FText& Line)
{
	if (NPCNameText)
	{
		NPCNameText->SetText(NPCName);
	}
	if (DialogueLineText)
	{
		DialogueLineText->SetText(Line);
	}
}
