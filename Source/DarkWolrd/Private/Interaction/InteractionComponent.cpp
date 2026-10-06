// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/InteractionComponent.h"
#include "NPC/NPCBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInteractableTarget();
}


// 조준선이 NPC를 향하고 있고, 그 NPC가 상호작용 범위 안에 있으면 CurrentInteractableNPC를 채움. 아니면 nullptr
void UInteractionComponent::UpdateInteractableTarget()
{
	CurrentInteractableNPC = nullptr;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	UCameraComponent* OwnerCamera = OwnerActor->FindComponentByClass<UCameraComponent>();
	const FVector TraceStart = OwnerCamera ? OwnerCamera->GetComponentLocation() : OwnerActor->GetActorLocation();
	const FVector TraceForward = OwnerCamera ? OwnerCamera->GetComponentRotation().Vector() : OwnerActor->GetActorForwardVector();
	const FVector TraceEnd = TraceStart + TraceForward * InteractionTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)) return;

	ANPCBase* HitNPC = Cast<ANPCBase>(Hit.GetActor());
	if (!HitNPC) return;

	const float Distance = FVector::Dist(OwnerActor->GetActorLocation(), HitNPC->GetActorLocation());
	if (Distance <= InteractionRange)
	{
		CurrentInteractableNPC = HitNPC;
	}
}


// F 입력 처리: 상호작용 가능한 NPC가 있으면 다음 대사를 가져와 반환
bool UInteractionComponent::TryInteract(FText& OutNPCName, FText& OutLine, bool& bOutIsLastLine)
{
	if (!CurrentInteractableNPC) return false;

	CurrentInteractableNPC->StartFacingPlayer(GetOwner());

	OutNPCName = CurrentInteractableNPC->NPCDisplayName;
	OutLine = CurrentInteractableNPC->GetNextDialogueLine(bOutIsLastLine);
	return true;
}
