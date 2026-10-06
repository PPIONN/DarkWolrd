// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/DungeonPortal.h"
#include "Character/Rabbit.h"
#include "DarkWolrdGameInstance.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADungeonPortal::ADungeonPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetSphereRadius(120.f);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = Trigger;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TravelMessage = NSLOCTEXT("Dungeon", "PortalTravel", "이동합니다...");
}


void ADungeonPortal::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ADungeonPortal::HandleTriggerBeginOverlap);
	SetPortalActive(bStartsActive);
}


// 켜기/끄기: 꺼진 포털은 보이지 않고 겹침 판정도 하지 않음. 켜는 순간 이미 위에 서 있는 플레이어도 바로 반응함
void ADungeonPortal::SetPortalActive(bool bActive)
{
	bIsActive = bActive;
	SetActorHiddenInGame(!bActive);
	Trigger->SetGenerateOverlapEvents(bActive);

	if (bActive)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, ARabbit::StaticClass());
		if (Overlapping.Num() > 0)
		{
			HandleTriggerBeginOverlap(Trigger, Overlapping[0], nullptr, 0, false, FHitResult());
		}
	}
}


void ADungeonPortal::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive || bTraveling) return;

	ARabbit* Rabbit = Cast<ARabbit>(OtherActor);
	if (!Rabbit || Rabbit->IsDead()) return;

	if (!bReturnToPreviousLevel && TargetLevel.IsNull())
	{
		UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Dungeon", "PortalNoTarget", "포털의 TargetLevel이 지정되지 않았습니다"), 3.f);
		return;
	}

	bTraveling = true;
	TravelingPlayer = Rabbit;
	UGameAnnouncementSubsystem::AnnounceInWorld(this, TravelMessage, FMath::Max(TravelDelay, 1.f));
	GetWorldTimerManager().SetTimer(TravelTimerHandle, this, &ADungeonPortal::Travel, FMath::Max(TravelDelay, 0.05f), false);
}


// 이동: 플레이어 상태를 게임 인스턴스에 담고, 입구 포털이면 "이 포털 앞"을 돌아올 위치로 기억한 뒤 레벨 열기
void ADungeonPortal::Travel()
{
	UDarkWolrdGameInstance* GameInstance = Cast<UDarkWolrdGameInstance>(GetGameInstance());
	ARabbit* Rabbit = TravelingPlayer.Get();

	if (!GameInstance || !Rabbit || Rabbit->IsDead())
	{
		bTraveling = false;
		return;
	}

	if (bReturnToPreviousLevel)
	{
		GameInstance->PrepareLevelTravel(Rabbit, /*bSetReturnPoint=*/false, FTransform::Identity);
		if (!GameInstance->TravelToReturnPoint())
		{
			UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Dungeon", "PortalNoReturn", "돌아갈 곳이 없습니다 (입구 포털로 들어오지 않음)"), 3.f);
			bTraveling = false;
		}
		return;
	}

	const FVector ReturnLocation = GetActorTransform().TransformPosition(ReturnOffset);
	const FRotator ReturnRotation(0.f, (ReturnLocation - GetActorLocation()).Rotation().Yaw, 0.f);
	GameInstance->PrepareLevelTravel(Rabbit, /*bSetReturnPoint=*/true, FTransform(ReturnRotation, ReturnLocation));

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TargetLevel);
}
