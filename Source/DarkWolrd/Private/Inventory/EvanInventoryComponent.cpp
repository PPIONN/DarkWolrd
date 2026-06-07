#include "Inventory/EvanInventoryComponent.h"


UEvanInventoryComponent::UEvanInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}
void UEvanInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    // 1. 먼저 세상의 아이템 정보를 도감(ItemLibrary)에 등록합니다.
    FItemDetail HPPotionDetail;
    HPPotionDetail.Tooltip = TEXT("체력을 50 회복시켜줍니다.");
    HPPotionDetail.RequiredTitle = TEXT("초보 모험가");

    ItemLibrary.Add(TEXT("하급 포션"), HPPotionDetail);

    // 2. 도감이 잘 채워졌는지 확인하고, 가방에 넣어줍니다.
    if (ItemLibrary.Contains(TEXT("하급 포션")))
    {
        // 도감에 있는 아이템일 때만 가방에 추가!
        AddItem(TEXT("하급 포션"), 5);
    }
}

void UEvanInventoryComponent::AddItem(AHP_Potion* PotionActor, int32 Count)
{
    // 1. 안전성 검사: 전달받은 액터가 진짜 존재하는지 확인
    if (!PotionActor) return;

    // 2. 액터가 들고 있는 변수들(우리가 아까 만든 것들)을 가져옵니다.
    FString Name = PotionActor->PotionName;
    FString Tooltip = PotionActor->PotionTooltip;
    float HealValue = PotionActor->HealHP;

    // 3. 도감(Library)에 이 아이템 정보가 없다면 새로 등록해줍니다. (자동 도감 등록)
    if (!ItemLibrary.Contains(Name))
    {
        FItemDetail NewDetail;
        NewDetail.Tooltip = Tooltip;
        NewDetail.RequiredTitle = TEXT("초보 모험가"); // 기본 칭호 설정
        // 만약 FItemDetail에 HealHP 변수를 만드셨다면 여기서 복사: NewDetail.HealHP = HealValue;

        ItemLibrary.Add(Name, NewDetail);
        UE_LOG(LogTemp, Log, TEXT("새로운 아이템을 도감에 등록했습니다: %s"), *Name);
    }

    // 4. 가방(MyBag)에 추가
    MyBag.Add({ Name, Count });

    UE_LOG(LogTemp, Warning, TEXT("가방에 [%s] %d개를 넣었습니다. (설명: %s)"), *Name, Count, *Tooltip);
}

void UEvanInventoryComponent::UseItem(FString Name)
{
    // 도감에 있는지 확인
    if (!ItemLibrary.Contains(Name)) return;

    FItemDetail& Detail = ItemLibrary[Name];

    // 칭호 체크
    if (MyTitles.Contains(Detail.RequiredTitle))
    {
        // Detail.Description -> Detail.Tooltip으로 수정 (헤더와 일치)
        UE_LOG(LogTemp, Warning, TEXT("%s 아이템 사용! 효과: %s"), *Name, *Detail.Tooltip);

        // TODO: 수량 감소 로직...
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("사용 불가! '%s' 칭호가 필요합니다."), *Detail.RequiredTitle);
    }
}

// [문자열 버전] 도감에 있는지 확인하고 가방에 넣음
void UEvanInventoryComponent::AddItem(FString Name, int32 Count)
{
    // 도감에 정보가 있을 때만 가방에 추가
    if (ItemLibrary.Contains(Name))
    {
        MyBag.Add({ Name, Count });
        UE_LOG(LogTemp, Warning, TEXT("가방에 %s %d개를 넣었습니다."), *Name, Count);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("도감에 %s 정보가 없습니다!"), *Name);
    }
}

// [액터 버전] 액터에서 정보를 뽑아낸 뒤, 위 함수를 재사용함
void UEvanInventoryComponent::AddItemFromActor(AHP_Potion* PotionActor, int32 Count)
{
    if (!PotionActor) return;

    FString Name = PotionActor->PotionName;

    // 도감에 없으면 액터의 정보로 도감에 먼저 등록
    if (!ItemLibrary.Contains(Name))
    {
        FItemDetail NewDetail;
        NewDetail.Tooltip = PotionActor->PotionTooltip;
        NewDetail.RequiredTitle = TEXT("초보 모험가");
        NewDetail.IconTexture = PotionActor->ItemIcon;
        ItemLibrary.Add(Name, NewDetail);
    }

    // 이미 만들어둔 이름 기반 AddItem 호출!
    AddItem(Name, Count);
}
