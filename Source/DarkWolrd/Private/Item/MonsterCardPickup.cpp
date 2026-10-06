// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/MonsterCardPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Companion/MonsterCompanionComponent.h"

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

	// CardMesh: 기본값은 비어있으며, 에디터/블루프린트에서 카드 모양 메시와 머티리얼을 지정해서 사용
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(PickupSphere);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// 게임 시작 시: 겹침 이벤트 바인딩
void AMonsterCardPickup::BeginPlay()
{
	Super::BeginPlay();

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
