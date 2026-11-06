// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/MonsterCardPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// 생성자: 습득 판정용 구체를 루트로, 카드 외형 메시를 부착
AMonsterCardPickup::AMonsterCardPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(60.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	SetRootComponent(PickupSphere);

	// CardMesh: 기본은 엔진 기본 큐브를 얇게 눌러 만든 카드 모양 판 (세로 50, 가로 35, 두께 3cm).
	// 블루프린트에서 다른 메시/머티리얼로 바꿔도 됨
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(PickupSphere);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardMesh->SetRelativeScale3D(FVector(0.03f, 0.35f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCardMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCardMesh.Succeeded())
	{
		CardMesh->SetStaticMesh(DefaultCardMesh.Object);
	}

	// RotatingMovement: 제자리에서 빙글빙글 돌아 눈에 띄게 함
	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.f, 120.f, 0.f);

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(PickupSphere);
	GlowLight->SetIntensity(1500.f);
	GlowLight->SetAttenuationRadius(400.f);
	GlowLight->SetLightColor(FLinearColor(1.f, 0.8f, 0.35f));
	GlowLight->SetCastShadows(false);
}

// 게임 시작 시: 바닥 위로 옮기고 겹침 이벤트 바인딩
void AMonsterCardPickup::BeginPlay()
{
	Super::BeginPlay();

	// 공중에서 죽은 몬스터(박쥐 등)의 카드도 주울 수 있도록, 아래 바닥을 찾아 HoverHeight 높이에 띄움
	FHitResult Hit;
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 3000.f);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		SetActorLocation(Hit.Location + FVector(0.f, 0.f, HoverHeight));
	}

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AMonsterCardPickup::OnPickupSphereBeginOverlap);
}

// 습득 처리: 겹친 액터가 UMonsterCompanionComponent를 가지고 있으면 카드를 넘겨주고 스스로 파괴됨
void AMonsterCardPickup::OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	if (UMonsterCompanionComponent* CompanionComponent = OtherActor->FindComponentByClass<UMonsterCompanionComponent>())
	{
		CompanionComponent->AddCard(CardData);
		Destroy();
	}
}
