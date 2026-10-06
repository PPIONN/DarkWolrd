// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/DungeonManager.h"
#include "Dungeon/DungeonRoom.h"
#include "Dungeon/DungeonPortal.h"
#include "Monster/BaseMonster.h"
#include "Stats/CharacterStatComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"

ADungeonManager::ADungeonManager()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;

	EnterAnnouncement = NSLOCTEXT("Dungeon", "Enter", "{Dungeon}에 입장했습니다");
	ClearAnnouncement = NSLOCTEXT("Dungeon", "Clear", "{Dungeon} 클리어! ({Time})  {Reward}");
}


// 시작: 입장 알림, 모든 방의 시작 이벤트(시간 측정용)와 마지막 방 클리어 이벤트 구독
void ADungeonManager::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ADungeonRoom> It(GetWorld()); It; ++It)
	{
		It->OnRoomStarted.AddDynamic(this, &ADungeonManager::HandleRoomStarted);
	}

	if (FinalRoom)
	{
		FinalRoom->OnRoomCleared.AddDynamic(this, &ADungeonManager::HandleFinalRoomCleared);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: FinalRoom이 지정되지 않아 던전 클리어 판정을 할 수 없음"), *GetName());
	}

	if (!EnterAnnouncement.IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Dungeon"), DungeonName);
		UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(EnterAnnouncement, Args), 4.f);
	}
}


void ADungeonManager::HandleRoomStarted(ADungeonRoom* Room)
{
	if (StartTime < 0.f)
	{
		StartTime = GetWorld()->GetTimeSeconds();
	}
}


float ADungeonManager::GetElapsedTime() const
{
	if (StartTime < 0.f) return 0.f;
	return (bCleared ? ClearTime : GetWorld()->GetTimeSeconds()) - StartTime;
}


// 클리어: 보상 지급 → 클리어 시간과 보상 요약 알림 → 출구 포털 켜기
void ADungeonManager::HandleFinalRoomCleared(ADungeonRoom* Room)
{
	if (bCleared) return;
	bCleared = true;
	ClearTime = GetWorld()->GetTimeSeconds();

	TArray<FString> RewardParts;
	if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		if (UCharacterStatComponent* Stat = Player->FindComponentByClass<UCharacterStatComponent>())
		{
			if (ClearRewardEXP > 0)
			{
				Stat->AddExperience(ClearRewardEXP);
				RewardParts.Add(FString::Printf(TEXT("경험치 +%d"), ClearRewardEXP));
			}
			if (ClearRewardMoney > 0)
			{
				Stat->AddMoney(ClearRewardMoney);
				RewardParts.Add(FString::Printf(TEXT("돈 +%lld"), ClearRewardMoney));
			}
		}

		if (UMonsterCompanionComponent* Companion = Player->FindComponentByClass<UMonsterCompanionComponent>())
		{
			for (const FDungeonCardReward& Reward : RewardCards)
			{
				if (!Reward.MonsterClass || FMath::FRand() > Reward.Chance) continue;

				FMonsterCardEntry Card;
				Card.MonsterClass = Reward.MonsterClass;
				Companion->AddCard(Card);

				FString CardName = Reward.MonsterClass->GetName();
				CardName.RemoveFromEnd(TEXT("_C"));
				RewardParts.Add(FString::Printf(TEXT("%s 카드"), *CardName));
			}
		}
	}

	const int32 TotalSeconds = FMath::RoundToInt(GetElapsedTime());
	FFormatNamedArguments Args;
	Args.Add(TEXT("Dungeon"), DungeonName);
	Args.Add(TEXT("Time"), FText::FromString(FString::Printf(TEXT("%d:%02d"), TotalSeconds / 60, TotalSeconds % 60)));
	Args.Add(TEXT("Reward"), FText::FromString(FString::Join(RewardParts, TEXT(", "))));
	UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(ClearAnnouncement, Args), 8.f);

	if (ExitPortal)
	{
		ExitPortal->SetPortalActive(true);
	}
}
