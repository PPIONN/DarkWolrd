// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/DungeonRoom.h"
#include "Dungeon/DungeonDoor.h"
#include "Monster/BaseMonster.h"
#include "AI/WildMonsterAIComponent.h"
#include "Character/Rabbit.h"
#include "Companion/MonsterCompanionComponent.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

ADungeonRoom::ADungeonRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomTrigger"));
	RoomTrigger->SetBoxExtent(FVector(800.f, 800.f, 300.f));
	RoomTrigger->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = RoomTrigger;

	ClearAnnouncement = NSLOCTEXT("Dungeon", "RoomClear", "{Room} 클리어!");
}


void ADungeonRoom::BeginPlay()
{
	Super::BeginPlay();

	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoom::HandleTriggerBeginOverlap);
}


// 플레이어가 들어옴: 대기 중이고 선행 방 조건을 만족하면 시작
void ADungeonRoom::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (RoomState != EDungeonRoomState::Waiting) return;
	if (RequiredRoom && RequiredRoom->GetRoomState() != EDungeonRoomState::Cleared) return;

	if (ARabbit* Rabbit = Cast<ARabbit>(OtherActor))
	{
		if (!Rabbit->IsDead())
		{
			StartRoom(Rabbit);
		}
	}
}


// 시작: 열려 있던 문을 기억해두고 전부 닫은 뒤, 밖에 남은 소환수를 불러들이고 첫 웨이브 예약
void ADungeonRoom::StartRoom(ARabbit* InChallenger)
{
	if (RoomState != EDungeonRoomState::Waiting) return;

	RoomState = EDungeonRoomState::InProgress;
	CurrentWaveIndex = INDEX_NONE;
	Challenger = InChallenger;

	if (InChallenger)
	{
		InChallenger->OnDied.AddDynamic(this, &ADungeonRoom::HandleChallengerDied);
	}

	DoorsOpenBeforeStart.Reset();
	for (ADungeonDoor* Door : Doors)
	{
		if (!Door) continue;
		if (Door->IsOpen())
		{
			DoorsOpenBeforeStart.Add(Door);
		}
		Door->SetOpen(false);
	}

	GatherSummonsToChallenger();
	OnRoomStarted.Broadcast(this);
	ScheduleNextWave();
}


void ADungeonRoom::ScheduleNextWave()
{
	const int32 NextWaveIndex = CurrentWaveIndex + 1;
	if (!Waves.IsValidIndex(NextWaveIndex))
	{
		ClearRoom();
		return;
	}

	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &ADungeonRoom::SpawnScheduledWave,
		FMath::Max(Waves[NextWaveIndex].StartDelay, 0.05f), false);
}


void ADungeonRoom::SpawnScheduledWave()
{
	SpawnWave(CurrentWaveIndex + 1);
}


// 웨이브 소환: 방 전투용으로 넓은 감지·귀환 없음 설정을 스폰 전에 넣어두고, 사망 이벤트로 남은 수를 셈
void ADungeonRoom::SpawnWave(int32 WaveIndex)
{
	if (RoomState != EDungeonRoomState::InProgress || !Waves.IsValidIndex(WaveIndex)) return;

	CurrentWaveIndex = WaveIndex;
	const FDungeonWave& Wave = Waves[WaveIndex];

	if (!Wave.Announcement.IsEmpty())
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, Wave.Announcement, 4.f);
	}

	int32 SpawnIndex = 0;
	for (const FDungeonSpawnEntry& Entry : Wave.Monsters)
	{
		if (!Entry.MonsterClass) continue;

		const ABaseMonster* Defaults = Entry.MonsterClass->GetDefaultObject<ABaseMonster>();
		const float HalfHeight = Defaults ? Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;

		for (int32 Copy = 0; Copy < Entry.Count; ++Copy, ++SpawnIndex)
		{
			const FVector Ground = PickSpawnLocation(SpawnIndex);
			const FVector ToChallenger = Challenger.IsValid() ? Challenger->GetActorLocation() - Ground : FVector::ForwardVector;
			const FTransform SpawnTransform(FRotator(0.f, ToChallenger.Rotation().Yaw, 0.f), Ground + FVector(0.f, 0.f, HalfHeight));

			ABaseMonster* Monster = GetWorld()->SpawnActorDeferred<ABaseMonster>(Entry.MonsterClass, SpawnTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!Monster) continue;

			Monster->Faction = ECombatFaction::Wild;
			Monster->Level = Entry.Level;
			if (Monster->WildAIComponent)
			{
				Monster->WildAIComponent->bAggressive = true;
				Monster->WildAIComponent->AggroRadius = MonsterAggroRadius;
				Monster->WildAIComponent->LeashRadius = 100000.f;
				Monster->WildAIComponent->bHealOnReturn = false;
			}
			Monster->FinishSpawning(SpawnTransform);

			Monster->OnMonsterDied.AddDynamic(this, &ADungeonRoom::HandleMonsterDied);
			AliveMonsters.Add(Monster);
		}
	}

	// 소환에 전부 실패했으면(클래스 미지정 등) 막히지 않도록 다음으로 넘어감
	if (GetAliveMonsterCount() == 0)
	{
		ScheduleNextWave();
	}
}


void ADungeonRoom::HandleMonsterDied(ABaseMonster* DeadMonster)
{
	AliveMonsters.RemoveAll([DeadMonster](const TWeakObjectPtr<ABaseMonster>& Weak) { return !Weak.IsValid() || Weak.Get() == DeadMonster; });

	if (RoomState == EDungeonRoomState::InProgress && GetAliveMonsterCount() == 0)
	{
		ScheduleNextWave();
	}
}


int32 ADungeonRoom::GetAliveMonsterCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ABaseMonster>& Weak : AliveMonsters)
	{
		if (Weak.IsValid() && Weak->IsAlive())
		{
			++Count;
		}
	}
	return Count;
}


// 클리어: 문을 전부 열고 알림
void ADungeonRoom::ClearRoom()
{
	RoomState = EDungeonRoomState::Cleared;
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	UnbindChallenger();

	for (ADungeonDoor* Door : Doors)
	{
		if (Door)
		{
			Door->SetOpen(true);
		}
	}

	if (!ClearAnnouncement.IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Room"), RoomName.IsEmpty() ? NSLOCTEXT("Dungeon", "DefaultRoomName", "방") : RoomName);
		UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(ClearAnnouncement, Args), 3.f);
	}

	OnRoomCleared.Broadcast(this);
}


// 초기화: 남은 몬스터를 없애고, 시작 전에 열려 있던 문(입구)만 다시 열어 처음 상태로 되돌림
void ADungeonRoom::ResetRoom()
{
	if (RoomState != EDungeonRoomState::InProgress) return;

	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	UnbindChallenger();

	for (const TWeakObjectPtr<ABaseMonster>& Weak : AliveMonsters)
	{
		if (ABaseMonster* Monster = Weak.Get())
		{
			Monster->OnMonsterDied.RemoveDynamic(this, &ADungeonRoom::HandleMonsterDied);
			Monster->Destroy();
		}
	}
	AliveMonsters.Reset();

	for (const TWeakObjectPtr<ADungeonDoor>& Door : DoorsOpenBeforeStart)
	{
		if (Door.IsValid())
		{
			Door->SetOpen(true);
		}
	}

	RoomState = EDungeonRoomState::Waiting;
	CurrentWaveIndex = INDEX_NONE;
}


void ADungeonRoom::HandleChallengerDied()
{
	ResetRoom();
}


void ADungeonRoom::UnbindChallenger()
{
	if (ARabbit* Rabbit = Challenger.Get())
	{
		Rabbit->OnDied.RemoveDynamic(this, &ADungeonRoom::HandleChallengerDied);
	}
	Challenger.Reset();
}


FVector ADungeonRoom::PickSpawnLocation(int32 SpawnIndex) const
{
	TArray<const AActor*> ValidPoints;
	for (const AActor* Point : SpawnPoints)
	{
		if (Point)
		{
			ValidPoints.Add(Point);
		}
	}

	// 지점이 있으면 돌아가며 쓰되, 같은 지점에 여러 마리가 겹치지 않게 조금씩 흩뿌림
	if (ValidPoints.Num() > 0)
	{
		const FVector Base = ValidPoints[SpawnIndex % ValidPoints.Num()]->GetActorLocation();
		const int32 Round = SpawnIndex / ValidPoints.Num();
		if (Round == 0) return Base;

		const float Angle = Round * 2.3f;
		return Base + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (80.f * Round);
	}

	// 지점이 없으면 방 상자 안쪽에서 내비메시 위의 아무 곳 (상자 바닥 높이 기준)
	const FVector Center = RoomTrigger->GetComponentLocation();
	const FVector Extent = RoomTrigger->GetScaledBoxExtent();
	const FVector Floor(Center.X, Center.Y, Center.Z - Extent.Z);

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Candidate;
		if (NavSys->GetRandomReachablePointInRadius(Floor, FMath::Min(Extent.X, Extent.Y) * 0.8f, Candidate))
		{
			return Candidate.Location;
		}
	}
	return Floor + FVector(FMath::FRandRange(-0.6f, 0.6f) * Extent.X, FMath::FRandRange(-0.6f, 0.6f) * Extent.Y, 0.f);
}


// 소환수 불러들이기: 방 상자 밖에 있는 소환수를 플레이어 바로 옆으로 순간이동 (닫힌 문 밖에 갇히지 않게)
void ADungeonRoom::GatherSummonsToChallenger()
{
	ARabbit* Rabbit = Challenger.Get();
	UMonsterCompanionComponent* Companion = Rabbit ? Rabbit->FindComponentByClass<UMonsterCompanionComponent>() : nullptr;
	if (!Companion) return;

	const FBox RoomBox = RoomTrigger->Bounds.GetBox();
	int32 Index = 0;
	for (ABaseMonster* Summon : Companion->ActiveSummons)
	{
		if (!IsValid(Summon) || RoomBox.IsInsideXY(Summon->GetActorLocation())) continue;

		const float Angle = 2.f * PI * (Index++) / 3.f;
		const FVector Offset(FMath::Cos(Angle) * 150.f, FMath::Sin(Angle) * 150.f, 0.f);
		Summon->TeleportTo(Rabbit->GetActorLocation() + Offset, Summon->GetActorRotation());
	}
}
