// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Rabbit.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"


// 생성자: 스프링암·카메라 컴포넌트를 만들고 캐릭터 이동 방식을 설정
ARabbit::ARabbit()
{
	PrimaryActorTick.bCanEverTick = true;

	// SpringArm 설정
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	// Camera 설정
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 캐릭터가 컨트롤러 회전을 따르지 않고 이동 방향으로 자동 회전
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
}


// 게임 시작 시 한 번 호출: 플레이어 컨트롤러에 입력 매핑 컨텍스트를 등록
void ARabbit::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}


// 매 프레임 호출: 현재는 별도 로직 없이 부모 Tick만 실행
void ARabbit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


// 입력 바인딩: 각 입력 액션을 해당 함수에 연결
void ARabbit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction,      ETriggerEvent::Triggered,  this, &ARabbit::Move);
		EIC->BindAction(LookAction,      ETriggerEvent::Triggered,  this, &ARabbit::Look);
		EIC->BindAction(JumpAction,      ETriggerEvent::Started,    this, &ACharacter::Jump);
		EIC->BindAction(JumpAction,      ETriggerEvent::Completed,  this, &ACharacter::StopJumping);
		EIC->BindAction(InventoryAction, ETriggerEvent::Started,    this, &ARabbit::ToggleInventory);
	}
}


// 이동 처리: 입력값(앞뒤/좌우)을 카메라 방향 기준으로 변환하여 캐릭터를 움직임
void ARabbit::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller) return;

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection,   MovementVector.X);
}


// 시점 회전: 마우스 움직임으로 카메라를 상하좌우로 돌림
void ARabbit::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller) return;

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(-LookAxisVector.Y);
}


// 인벤토리 토글: 닫혀있으면 열고(마우스 커서 활성화), 열려있으면 닫음(게임 조작 복귀)
void ARabbit::ToggleInventory()
{
	if (!InventoryWidgetClass) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (!InventoryWidgetNative || !InventoryWidgetNative->IsInViewport())
	{
		if (!InventoryWidgetNative)
		{
			InventoryWidgetNative = CreateWidget<UUserWidget>(PC, InventoryWidgetClass);
		}

		if (InventoryWidgetNative)
		{
			InventoryWidgetNative->AddToViewport();

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(InventoryWidgetNative->GetCachedWidget());
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
	else
	{
		InventoryWidgetNative->RemoveFromParent();

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}
