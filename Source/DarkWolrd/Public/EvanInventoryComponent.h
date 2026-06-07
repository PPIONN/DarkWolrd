// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HP_Potion.h"
#include "EvanInventoryComponent.generated.h"

// [아이템 기본 정보] 가방(TArray)에 담길 데이터
USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName; // 아이템 이름

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 0; // 아이템 개수
};
// [아이템 상세 정보] 도감(TMap)에 저장될 데이터
USTRUCT(BlueprintType)
struct FItemDetail
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Tooltip; // 아이템 설명

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RequiredTitle; // 사용에 필요한 칭호

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* IconTexture;
};
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DARKWOLRD_API UEvanInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEvanInventoryComponent();

protected:
    virtual void BeginPlay() override;

public:
    // 1. 가방 (필수: TArray)
    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TArray<FItemData> MyBag;

    // 2. 아이템 도감 (필수: TMap) - 이름으로 정보를 찾음
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TMap<FString, FItemDetail> ItemLibrary;

    // 3. 획득한 칭호 (필수: TSet) - 중복 없는 수집품
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
    TSet<FString> MyTitles;

    // 아이템 추가 함수
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(AHP_Potion* PotionActor, int32 Count);

    // 아이템 사용 함수 (칭호 체크 포함)
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UseItem(FString Name);

    // 기존 함수는 문자열을 받도록 되돌립니다. (도감 검색용)
    void AddItem(FString Name, int32 Count);

    // 액터를 직접 주웠을 때를 위한 전용 함수를 하나 더 만듭니다. (오버로딩)
    void AddItemFromActor(AHP_Potion* PotionActor, int32 Count);
};
