// Fill out your copyright notice in the Description page of Project Settings.

#include "Rabbit.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"


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


void ARabbit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


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


void ARabbit::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller) return;

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(-LookAxisVector.Y);
}


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
