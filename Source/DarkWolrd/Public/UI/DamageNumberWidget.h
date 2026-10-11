// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

// UDamageNumberWidget: 피해를 받은 위치에 잠깐 떠오르는 피해 숫자 (ADamageNumberActor가 띄움).
// 위젯 BP 없이도 동작하도록, 디자이너에서 배치한 위젯이 없으면 외곽선 있는 텍스트를 코드로 만듦.
// 글꼴 등을 꾸미려면 이 클래스를 부모로 위젯 BP를 만들고 TextBlock(이름 DamageText)을 배치한 뒤 ADamageNumberActor의 WidgetClass에 지정
UCLASS()
class DARKWOLRD_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// SetDamage: 표시할 숫자, 색, 글자 크기
	void SetDamage(float Damage, const FLinearColor& Color, int32 FontSize);

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DamageText;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	// ApplyToText: 저장해 둔 숫자·색·크기를 텍스트에 반영 (위젯이 만들어지기 전에 SetDamage가 불려도 되도록)
	void ApplyToText();

	float DamageValue = 0.f;
	FLinearColor TextColor = FLinearColor::White;
	int32 TextSize = 20;
};
