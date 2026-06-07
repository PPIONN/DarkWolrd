// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// ACharacter: 언리얼 엔진의 기본 캐릭터 클래스. 이동, 점프 등 기본 기능이 내장되어 있음
#include "GameFramework/Character.h"
// UUserWidget: 화면에 표시되는 UI 위젯의 기본 클래스
#include "Blueprint/UserWidget.h"
// FInputActionValue: 입력 액션의 값(방향, 세기 등)을 담는 구조체
#include "InputActionValue.h"
#include "Rabbit.generated.h"

// 실제 정의는 각 헤더에 있지만, 포인터로만 사용하기 위해 미리 선언 (컴파일 속도 향상)
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;


// UCLASS: 이 클래스가 언리얼 엔진의 리플렉션 시스템에 등록되도록 하는 매크로
// ARabbit: 플레이어가 직접 조종하는 토끼 캐릭터. 'A'는 Actor(게임 세상에 존재하는 물체)를 의미
UCLASS()
class DARKWOLRD_API ARabbit : public ACharacter
{
	// GENERATED_BODY: 언리얼이 자동으로 필요한 코드를 생성해주는 매크로 (반드시 있어야 함)
	GENERATED_BODY()

public:
	// 생성자: 캐릭터가 처음 만들어질 때 실행. 카메라·스프링암 부착 및 이동 방식 설정
	ARabbit();

protected:
	// BeginPlay: 캐릭터가 게임 세상에 처음 등장할 때 한 번만 호출되는 함수
	virtual void BeginPlay() override;

	// Move: WASD 또는 조이스틱 입력을 받아 캐릭터를 앞뒤좌우로 이동시키는 함수
	void Move(const FInputActionValue& Value);
	// Look: 마우스 움직임을 받아 카메라 시점을 상하좌우로 회전시키는 함수
	void Look(const FInputActionValue& Value);
	// ToggleInventory: 인벤토리 UI를 열거나 닫는 함수
	void ToggleInventory();

public:
	// Tick: 매 프레임마다 자동으로 호출되는 함수. DeltaTime = 이전 프레임과의 시간 간격(초)
	virtual void Tick(float DeltaTime) override;
	// SetupPlayerInputComponent: 키보드·마우스 입력 액션을 각 함수에 연결하는 함수
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// UPROPERTY: 이 변수를 언리얼 에디터에서 확인·관리할 수 있게 해주는 매크로
	// VisibleAnywhere: 에디터 어디서든 값을 볼 수 있음 (직접 수정은 불가)
	// SpringArm: 카메라를 캐릭터 등 뒤에 띄워주는 팔. 벽에 닿으면 카메라를 자동으로 당겨줌
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* SpringArm;

	// Camera: 플레이어 시점을 담당하는 카메라 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* Camera;

	// EditAnywhere: 에디터와 블루프린트 양쪽에서 값을 자유롭게 수정 가능
	// DefaultMappingContext: 어떤 키가 어떤 동작에 대응되는지 정의하는 입력 매핑 테이블
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// MoveAction: 이동 입력(WASD 등)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	// LookAction: 시점 회전 입력(마우스 이동)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	// JumpAction: 점프 입력(스페이스바 등)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	// InventoryAction: 인벤토리 열기/닫기 입력과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InventoryAction;

	// TSubclassOf<UUserWidget>: UUserWidget을 상속한 클래스(주로 블루프린트 UI)만 받는 타입
	// InventoryWidgetClass: 에디터에서 지정할 인벤토리 UI의 블루프린트 클래스
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	// InventoryWidgetNative: 실제로 화면에 생성된 인벤토리 위젯 객체 (없으면 nullptr)
	UPROPERTY()
	UUserWidget* InventoryWidgetNative;
};
