// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/MonsterSpawner.h"
#include "Monster/BaseMonster.h"
#include "Components/BillboardComponent.h"
#include "Components/DrawSphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 에디터 뷰포트에서 보이고 클릭할 수 있도록 아이콘을 루트로 둠 (게임 중에는 숨겨짐)
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	RootComponent = Billboard;

	// 스폰 반경을 에디터에서 노란 구로 보여줌 (게임 중에는 숨김, 충돌 없음)
	RadiusPreview = CreateDefaultSubobject<UDrawSphereComponent>(TEXT("RadiusPreview"));
	RadiusPreview->SetupAttachment(RootComponent);
	RadiusPreview->SetSphereRadius(SpawnRadius);
	RadiusPreview->ShapeColor = FColor(255, 200, 0);
	RadiusPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RadiusPreview->SetHiddenInGame(true);
	RadiusPreview->bIsEditorOnly = true;
}


// 에디터에서 SpawnRadius를 바꾸면 노란 구 크기도 바로 따라감
void AMonsterSpawner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (RadiusPreview)
	{
		RadiusPreview->SetSphereRadius(SpawnRadius);
	}
}


// 게임 시작: 곧바로 MaxAlive마리를 채우고, 이후 0.5초마다 빈자리를 확인
void AMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!PickMonsterClass())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: MonsterTypes에 몬스터가 지정되지 않아 스폰하지 않음"), *GetName());
	}
	GetWorldTimerManager().SetTimer(CheckTimerHandle, this, &AMonsterSpawner::CheckRespawn, 0.5f, true, 0.1f);
}


void AMonsterSpawner::CheckRespawn()
{
	// 사망 이벤트 없이 사라진 몬스터(레벨 이동, 직접 삭제 등)도 목록에서 정리
	AliveMonsters.RemoveAll([](const TWeakObjectPtr<ABaseMonster>& Monster)
	{
		return !Monster.IsValid() || !Monster->IsAlive();
	});
	if (!bActive || !PickMonsterClass()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	const int32 FreeSlots = MaxAlive - AliveMonsters.Num();

	// 다시 나올 시간이 된 자리 + 예약 없이 비어 있는 자리(게임 시작 직후 등)를 채움
	const int32 DueCount = RespawnTimes.RemoveAll([Now](float RespawnTime) { return RespawnTime <= Now; });
	const int32 UnscheduledCount = FMath::Max(FreeSlots - DueCount - RespawnTimes.Num(), 0);
	const int32 SpawnCount = FMath::Clamp(DueCount + UnscheduledCount, 0, FreeSlots);

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		SpawnOne();
	}
}


// 스폰: 고른 종류를 반경 안 바닥 위에 세우고, 레벨·진영을 정한 뒤 사망 이벤트를 구독. 작게 튀어나오는 등장 연출
ABaseMonster* AMonsterSpawner::SpawnOne()
{
	UClass* MonsterClass = PickMonsterClass();
	if (!MonsterClass) return nullptr;

	const ABaseMonster* MonsterDefaults = MonsterClass->GetDefaultObject<ABaseMonster>();
	const float HalfHeight = MonsterDefaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector SpawnLocation;
	if (!FindSpawnLocation(HalfHeight, SpawnLocation)) return nullptr;

	const FTransform SpawnTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SpawnLocation);
	ABaseMonster* Monster = GetWorld()->SpawnActorDeferred<ABaseMonster>(MonsterClass, SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Monster) return nullptr;

	// 레벨은 BeginPlay에서 HP 계산에 쓰이므로 스폰을 마치기 전에 정함
	Monster->Faction = ECombatFaction::Wild;
	if (MinLevel > 0 || MaxLevel > 0)
	{
		const int32 LowLevel = FMath::Max(MinLevel, 1);
		Monster->Level = FMath::RandRange(LowLevel, FMath::Max(MaxLevel, LowLevel));
	}
	Monster->FinishSpawning(SpawnTransform);
	Monster->OnMonsterDied.AddDynamic(this, &AMonsterSpawner::HandleMonsterDied);
	AliveMonsters.Add(Monster);

	if (AppearDuration > 0.f)
	{
		Monster->PlaySummonAppear(AppearDuration);
	}

	const FVector FeetLocation = SpawnLocation - FVector(0.f, 0.f, HalfHeight);
	if (SpawnVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SpawnVFX, FeetLocation);
	}
	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, FeetLocation);
	}
	return Monster;
}


void AMonsterSpawner::HandleMonsterDied(ABaseMonster* DeadMonster)
{
	AliveMonsters.RemoveAll([DeadMonster](const TWeakObjectPtr<ABaseMonster>& Monster)
	{
		return !Monster.IsValid() || Monster.Get() == DeadMonster;
	});
	RespawnTimes.Add(GetWorld()->GetTimeSeconds() + RespawnDelay);
}


int32 AMonsterSpawner::GetAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ABaseMonster>& Monster : AliveMonsters)
	{
		if (Monster.IsValid() && Monster->IsAlive())
		{
			++Count;
		}
	}
	return Count;
}


UClass* AMonsterSpawner::PickMonsterClass() const
{
	float TotalWeight = 0.f;
	for (const FMonsterSpawnEntry& Entry : MonsterTypes)
	{
		if (Entry.MonsterClass && Entry.Weight > 0.f)
		{
			TotalWeight += Entry.Weight;
		}
	}
	if (TotalWeight <= 0.f) return nullptr;

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	UClass* LastValid = nullptr;
	for (const FMonsterSpawnEntry& Entry : MonsterTypes)
	{
		if (!Entry.MonsterClass || Entry.Weight <= 0.f) continue;

		LastValid = Entry.MonsterClass;
		Roll -= Entry.Weight;
		if (Roll <= 0.f) return LastValid;
	}
	return LastValid;
}


// 반경 안 무작위 지점에서 아래로 바닥(지형·스태틱 메시)을 찾고, 내비메시가 있으면 그 위로 맞춤. 몇 번 시도해도 바닥이 없으면 실패
bool AMonsterSpawner::FindSpawnLocation(float CapsuleHalfHeight, FVector& OutLocation) const
{
	const FVector Center = GetActorLocation();
	UNavigationSystemV1* NavSystem =FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MonsterSpawnerGround), false, this);
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(SpawnRadius);
		const FVector Candidate = Center + FVector(Offset.X, Offset.Y, 0.f);

		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByObjectType(Hit, Candidate + FVector(0.f, 0.f, 500.f), Candidate - FVector(0.f, 0.f, 2000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams))
		{
			continue;
		}

		FVector Ground = Hit.ImpactPoint;
		FNavLocation NavLocation;
		if (NavSystem && NavSystem->ProjectPointToNavigation(Ground, NavLocation, FVector(100.f, 100.f, 200.f)))
		{
			Ground = NavLocation.Location;
		}

		OutLocation = Ground + FVector(0.f, 0.f, CapsuleHalfHeight + 5.f);
		return true;
	}
	return false;
}
