// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameAnnouncementSubsystem.generated.h"

// FOnGameAnnouncement: 화면 알림 요청. HUD가 구독해서 AnnouncementText에 표시함
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameAnnouncement, const FText&, Message, float, Duration);

// UGameAnnouncementSubsystem: "필드보스가 나타났습니다!" 같은 화면 중앙 알림을 보내는 창구.
// 알림을 보내는 쪽(스포너, 던전, 대전)과 표시하는 쪽(HUD)이 서로를 몰라도 되게 중간에서 연결해줌.
// 월드마다 하나씩 자동으로 생기므로 GetWorld()->GetSubsystem<UGameAnnouncementSubsystem>()으로 바로 꺼내 쓰면 됨
UCLASS()
class DARKWOLRD_API UGameAnnouncementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// OnAnnouncement: 알림이 올 때마다 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Announcement")
	FOnGameAnnouncement OnAnnouncement;

	// Announce: 화면에 Message를 Duration초 동안 표시. 표시할 HUD가 없으면(AnnouncementText 미배치) 화면 왼쪽 위 디버그 글씨로 대신 표시
	UFUNCTION(BlueprintCallable, Category = "Announcement")
	void Announce(const FText& Message, float Duration = 5.f);

	// AnnounceInWorld: 월드 컨텍스트만 있으면 어디서든 바로 알림을 보내는 도우미
	static void AnnounceInWorld(const UObject* WorldContextObject, const FText& Message, float Duration = 5.f);
};
