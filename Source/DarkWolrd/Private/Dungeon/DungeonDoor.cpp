// Fill out your copyright notice in the Description page of Project Settings.

#include "Dungeon/DungeonDoor.h"
#include "Components/StaticMeshComponent.h"

ADungeonDoor::ADungeonDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(RootComponent);

	// 문은 열렸다 닫혔다 하므로 내비메시에 구멍을 내지 않게 함 (닫힌 문 너머로도 길이 이어져 있어야 열린 뒤 AI가 지나갈 수 있음)
	DoorMesh->SetCanEverAffectNavigation(false);
}


// 시작: 닫힌 자리를 기억하고 시작 상태로 즉시 맞춤 (움직이는 연출 없이)
void ADungeonDoor::BeginPlay()
{
	Super::BeginPlay();

	ClosedLocation = DoorMesh->GetRelativeLocation();
	bIsOpen = bStartsOpen;
	DoorMesh->SetRelativeLocation(bIsOpen ? ClosedLocation + OpenOffset : ClosedLocation);
}


void ADungeonDoor::SetOpen(bool bOpen)
{
	bIsOpen = bOpen;
	SetActorTickEnabled(true);
}


// 목표 위치까지 일정 속도로 이동하고, 도착하면 Tick을 꺼서 비용을 없앰
void ADungeonDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector Target = bIsOpen ? ClosedLocation + OpenOffset : ClosedLocation;
	const FVector Current = DoorMesh->GetRelativeLocation();
	const FVector NewLocation = FMath::VInterpConstantTo(Current, Target, DeltaTime, MoveSpeed);
	DoorMesh->SetRelativeLocation(NewLocation);

	if (NewLocation.Equals(Target, 0.5f))
	{
		DoorMesh->SetRelativeLocation(Target);
		SetActorTickEnabled(false);
	}
}
