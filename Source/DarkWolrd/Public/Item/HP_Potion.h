// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// AActor: 게임 세상에 배치할 수 있는 모든 오브젝트의 기본 클래스
#include "GameFramework/Actor.h"
#include "HP_Potion.generated.h"

// AHP_Potion: 체력을 회복시켜주는 포션 아이템 클래스
// 'A'는 Actor(게임 세상에 존재하는 물체)를 의미
UCLASS()
class DARKWOLRD_API AHP_Potion : public AActor
{
	// GENERATED_BODY: 언리얼 엔진이 자동으로 필요한 코드를 생성해주는 매크로
	GENERATED_BODY()

public:
	// 생성자: 포션 액터가 처음 생성될 때 기본값을 설정
	AHP_Potion();

protected:
	// BeginPlay: 포션이 게임 세상에 등장할 때 한 번 실행되는 함수
	virtual void BeginPlay() override;

public:
	// Tick: 매 프레임마다 자동으로 호출되는 함수 (현재는 별도 로직 없음)
	virtual void Tick(float DeltaTime) override;

	// EditAnywhere: 에디터와 블루프린트에서 값을 직접 수정 가능
	// BlueprintReadWrite: 블루프린트에서 이 변수를 읽고 쓸 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	FString PotionName = TEXT("하급 포션"); // 포션 이름

	// PotionTooltip: 인벤토리에서 포션 위에 마우스를 올렸을 때 표시될 설명 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	FString PotionTooltip = TEXT("체력을 50 회복시켜줍니다.");

	// ItemIcon: 인벤토리 슬롯에 표시될 포션 아이콘 이미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	UTexture2D* ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PotionData")
	float HealHP = 50.0f; // 포션 성능
};
