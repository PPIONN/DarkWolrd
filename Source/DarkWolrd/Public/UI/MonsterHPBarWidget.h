// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MonsterHPBarWidget.generated.h"

class UProgressBar;
class ABaseMonster;

// UMonsterHPBarWidget: 몬스터 머리 위 HP바 (ABaseMonster의 HPBarComponent가 띄움).
// 위젯 BP 없이 이 클래스만으로 동작하도록, 디자이너에서 배치한 위젯이 없으면 코드로 바 두 개를 만들어 씀.
// 모양을 꾸미려면 이 클래스를 부모로 위젯 BP를 만들고 ProgressBar 두 개(이름 HPBar, TrailBar)를 겹쳐 배치한 뒤
// 몬스터 BP의 HPBarComponent → Widget Class에 지정
UCLASS()
class DARKWOLRD_API UMonsterHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// SetMonster: 표시할 몬스터와 바 색을 지정
	void SetMonster(ABaseMonster* InMonster, const FLinearColor& InFillColor);

	// HPBar: 현재 HP
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HPBar;

	// TrailBar: 피해를 받으면 잠깐 남아 있다가 천천히 줄어드는 잔상 바 (HPBar 뒤에 겹쳐 둠, 없어도 됨)
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* TrailBar;

	// TrailDelay: 피해를 받은 뒤 잔상이 줄어들기 시작할 때까지의 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP Bar")
	float TrailDelay = 0.4f;

	// TrailSpeed: 잔상이 줄어드는 속도 (초당 전체 HP 대비 비율, 1 = 1초에 100%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HP Bar")
	float TrailSpeed = 0.8f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// GetHPPercent: 몬스터의 현재 HP 비율(0~1)
	float GetHPPercent() const;

	TWeakObjectPtr<ABaseMonster> Monster;
	FLinearColor FillColor = FLinearColor::Red;

	float LastPercent = 1.f;
	float TrailPercent = 1.f;
	float TrailHoldTimeLeft = 0.f;
};
