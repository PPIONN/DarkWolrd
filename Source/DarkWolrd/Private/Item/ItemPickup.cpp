// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/ItemPickup.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "UI/GameAnnouncementSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(45.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(PickupSphere);

	// 엔진 기본 구를 작게 줄인 구슬. 색은 BeginPlay에서 희귀도 색으로 칠함
	GemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GemMesh"));
	GemMesh->SetupAttachment(PickupSphere);
	GemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GemMesh->SetRelativeScale3D(FVector(0.18f));
	GemMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		GemMesh->SetStaticMesh(SphereMesh.Object);
	}

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(PickupSphere);
	GlowLight->SetIntensity(900.f);
	GlowLight->SetAttenuationRadius(220.f);
	GlowLight->SetCastShadows(false);
}


AItemPickup* AItemPickup::SpawnDrop(UWorld* World, UItemDefinition* InItem, int32 InCount, const FVector& Location)
{
	if (!World || !InItem || InCount <= 0) return nullptr;

	// 여러 개가 겹치지 않게 조금씩 흩뿌림
	const FVector2D Offset = FMath::RandPointInCircle(70.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.bDeferConstruction = true;
	AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FTransform(Location + FVector(Offset, 0.f)), Params);
	if (Pickup)
	{
		Pickup->Item = InItem;
		Pickup->Count = InCount;
		UGameplayStatics::FinishSpawningActor(Pickup, FTransform(Location + FVector(Offset, 0.f)));
	}
	return Pickup;
}


void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// 공중에서 죽은 몬스터(박쥐)의 아이템도 주울 수 있게 아래 바닥을 찾아 그 위에 띄움
	FHitResult Hit;
	const FVector Start = GetActorLocation();
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, Start - FVector(0.f, 0.f, 3000.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		SetActorLocation(Hit.Location + FVector(0.f, 0.f, HoverHeight));
	}
	BaseLocation = GetActorLocation();

	const FLinearColor Color = Item ? UItemDefinition::GetRarityColor(Item->Rarity) : FLinearColor::White;
	GlowLight->SetLightColor(Color);
	if (UMaterialInstanceDynamic* Material = GemMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemPickup::OnPickupSphereBeginOverlap);
	SetLifeSpan(180.f);
}


void AItemPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Age += DeltaTime;

	// 둥실둥실 떠 있고, 토끼가 가까우면 끌려감
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && Age >= PickupDelay)
	{
		const FVector Target = Player->GetActorLocation();
		if (FVector::Dist(Target, GetActorLocation()) <= MagnetRadius && !bFullWarned)
		{
			SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), Target, DeltaTime, MagnetSpeed));
			BaseLocation = GetActorLocation();
			return;
		}
	}
	SetActorLocation(BaseLocation + FVector(0.f, 0.f, 8.f * FMath::Sin(Age * 3.f)));
	GemMesh->AddLocalRotation(FRotator(0.f, 120.f * DeltaTime, 0.f));
}


void AItemPickup::OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryGiveTo(OtherActor);
}


void AItemPickup::TryGiveTo(AActor* OtherActor)
{
	UInventoryComponent* Inventory = OtherActor ? OtherActor->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory || !Item) return;

	const int32 Added = Inventory->AddItem(Item, Count);
	Count -= Added;
	if (Count <= 0)
	{
		Destroy();
		return;
	}
	if (!bFullWarned)
	{
		// 가득 차서 못 넣은 건 그 자리에 남김 (더 끌려오지 않게 해서 토끼를 졸졸 따라다니지 않음)
		bFullWarned = true;
		UGameAnnouncementSubsystem::AnnounceInWorld(this, NSLOCTEXT("Inventory", "Full", "인벤토리가 가득 찼습니다"), 2.5f);
	}
}
