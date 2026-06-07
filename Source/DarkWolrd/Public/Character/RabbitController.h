// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// APlayerController: 플레이어의 입력을 받아 캐릭터를 제어하는 컨트롤러 기본 클래스
#include "GameFramework/PlayerController.h"
#include "RabbitController.generated.h"

// ARabbitController: 토끼 캐릭터 전용 플레이어 컨트롤러
// 현재는 기본 APlayerController 기능만 사용하며, 추후 커스텀 로직 추가 예정
UCLASS()
class DARKWOLRD_API ARabbitController : public APlayerController
{
	// GENERATED_BODY: 언리얼 엔진이 자동으로 필요한 코드를 생성해주는 매크로
	GENERATED_BODY()

};
