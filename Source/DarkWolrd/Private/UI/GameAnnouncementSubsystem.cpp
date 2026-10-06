// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/GameAnnouncementSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UGameAnnouncementSubsystem::Announce(const FText& Message, float Duration)
{
	if (OnAnnouncement.IsBound())
	{
		OnAnnouncement.Broadcast(Message, Duration);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Orange, Message.ToString());
	}
}


void UGameAnnouncementSubsystem::AnnounceInWorld(const UObject* WorldContextObject, const FText& Message, float Duration)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (UGameAnnouncementSubsystem* Subsystem = World ? World->GetSubsystem<UGameAnnouncementSubsystem>() : nullptr)
	{
		Subsystem->Announce(Message, Duration);
	}
}
