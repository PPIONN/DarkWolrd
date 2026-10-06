// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/FieldBossSpawner.h"
#include "Monster/FieldBossMonster.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

AFieldBossSpawner::AFieldBossSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 에디터 뷰포트에서 보이고 클릭할 수 있도록 아이콘을 루트로 둠 (게임 중에는 숨겨짐)
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;

	SpawnAnnouncement = NSLOCTEXT("FieldBoss", "SpawnAnnouncement", "필드보스 {Boss}이(가) {Location}에 나타났습니다!");
	DefeatAnnouncement = NSLOCTEXT("FieldBoss", "DefeatAnnouncement", "필드보스 {Boss}을(를) 쓰러뜨렸습니다!");
}


// 게임 시작: 첫 등장 예약
void AFieldBossSpawner::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AFieldBossSpawner::SpawnFromTimer, FMath::Max(FirstSpawnDelay, 0.1f), false);
}


void AFieldBossSpawner::SpawnFromTimer()
{
	SpawnBossNow();
}


// 소환: 후보 지점을 골라 바닥 위로 캡슐 절반 높이만큼 띄워 스폰하고, 사망 이벤트를 구독한 뒤 등장 알림
AFieldBossMonster* AFieldBossSpawner::SpawnBossNow()
{
	if (IsValid(CurrentBoss) && CurrentBoss->IsAlive()) return CurrentBoss;

	if (!BossClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: BossClass가 지정되지 않아 필드보스를 소환할 수 없음"), *GetName());
		return nullptr;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	const int32 PointIndex = PickSpawnPointIndex();
	const AActor* PointActor = (PointIndex != INDEX_NONE) ? SpawnPoints[PointIndex].Point : nullptr;
	LastSpawnPointIndex = PointIndex;
	CurrentLocationName = (PointIndex != INDEX_NONE) ? SpawnPoints[PointIndex].LocationName : FText::GetEmpty();

	FVector SpawnLocation = PointActor ? PointActor->GetActorLocation() : GetActorLocation();
	const FRotator SpawnRotation(0.f, (PointActor ? PointActor->GetActorRotation() : GetActorRotation()).Yaw, 0.f);

	if (const AFieldBossMonster* BossDefaults = BossClass->GetDefaultObject<AFieldBossMonster>())
	{
		SpawnLocation.Z += BossDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AFieldBossMonster* Boss = GetWorld()->SpawnActorDeferred<AFieldBossMonster>(BossClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Boss) return nullptr;

	Boss->Faction = ECombatFaction::Wild;
	Boss->FinishSpawning(SpawnTransform);
	Boss->OnMonsterDied.AddDynamic(this, &AFieldBossSpawner::HandleBossDied);
	CurrentBoss = Boss;

	FFormatNamedArguments Args;
	Args.Add(TEXT("Boss"), Boss->GetBossDisplayName());
	Args.Add(TEXT("Location"), CurrentLocationName.IsEmpty() ? NSLOCTEXT("FieldBoss", "UnknownLocation", "어딘가") : CurrentLocationName);
	UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(SpawnAnnouncement, Args), AnnouncementDuration);

	return Boss;
}


// 보스 사망: 처치 알림을 띄우고 RespawnDelay 뒤 다시 등장 예약
void AFieldBossSpawner::HandleBossDied(ABaseMonster* DeadMonster)
{
	if (DeadMonster != CurrentBoss) return;

	FFormatNamedArguments Args;
	Args.Add(TEXT("Boss"), CurrentBoss->GetBossDisplayName());
	Args.Add(TEXT("Location"), CurrentLocationName);
	UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(DefeatAnnouncement, Args), AnnouncementDuration);

	CurrentBoss = nullptr;
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AFieldBossSpawner::SpawnFromTimer, FMath::Max(RespawnDelay, 0.1f), false);
}


int32 AFieldBossSpawner::PickSpawnPointIndex() const
{
	TArray<int32> Candidates;
	for (int32 PointIndex = 0; PointIndex < SpawnPoints.Num(); ++PointIndex)
	{
		if (SpawnPoints[PointIndex].Point)
		{
			Candidates.Add(PointIndex);
		}
	}
	if (Candidates.Num() == 0) return INDEX_NONE;

	// 후보가 여러 곳이면 직전에 나타났던 곳은 빼서 매번 다른 곳에 나타나게 함
	if (Candidates.Num() > 1)
	{
		Candidates.Remove(LastSpawnPointIndex);
	}
	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}


float AFieldBossSpawner::GetTimeUntilNextSpawn() const
{
	return GetWorldTimerManager().IsTimerActive(SpawnTimerHandle) ? GetWorldTimerManager().GetTimerRemaining(SpawnTimerHandle) : -1.f;
}
