// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Rabbit.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/AbilityComponent.h"
#include "Stats/CharacterStatComponent.h"
#include "Companion/MonsterCompanionComponent.h"
#include "Monster/BaseMonster.h"
#include "Monster/FieldBossSpawner.h"
#include "Arena/MonsterArena.h"
#include "DarkWolrdGameInstance.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "UI/HUDWidget.h"
#include "UI/CrosshairWidget.h"
#include "UI/DialogueWidget.h"
#include "Interaction/InteractionComponent.h"
#include "NPC/NPCBase.h"
#include "Components/CapsuleComponent.h"
#include "Character/GameFeelComponent.h"
#include "Character/RabbitFaceComponent.h"


// 생성자: 스프링암·카메라 컴포넌트를 만들고 캐릭터 이동 방식을 설정
ARabbit::ARabbit()
{
	PrimaryActorTick.bCanEverTick = true;

	// 조준점 위젯: 기본은 코드로 그린 조준점 (BP에서 다른 위젯으로 바꾸거나 비워서 끌 수 있음)
	CrosshairWidgetClass = UCrosshairWidget::StaticClass();

	// SpringArm 설정
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = ExploreArmLength;
	SpringArm->SocketOffset = ExploreSocketOffset;
	SpringArm->bUsePawnControlRotation = true;

	// 카메라 지연 추적: 캐릭터가 갑자기 순간이동해도 카메라가 그 자리에서 즉시 스냅되지 않고 부드럽게 뒤쫓아가게 함
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = NormalCameraLagSpeed;

	// Camera 설정
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 몸 회전: 평소에는 달리는 방향으로 돌고(팰월드식 탐험), 조준·락온·공격 직후에는 카메라 방향을 봄(옆걸음).
	// 실제 전환은 매 프레임 UpdateCameraAndFacing에서 함. 여기서는 탐험 모드로 시작
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, ExploreTurnRate, 0.f);

	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));
	StatComponent = CreateDefaultSubobject<UCharacterStatComponent>(TEXT("StatComponent"));
	CompanionComponent = CreateDefaultSubobject<UMonsterCompanionComponent>(TEXT("CompanionComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	GameFeelComponent = CreateDefaultSubobject<UGameFeelComponent>(TEXT("GameFeelComponent"));
	FaceComponent = CreateDefaultSubobject<URabbitFaceComponent>(TEXT("FaceComponent"));

	// 조작감: 빨리 출발하고 빨리 멈추며, 공중에서도 방향을 어느 정도 틀 수 있게 (엔진 기본값은 가속·감속 2048, 공중 조작 0.05)
	GetCharacterMovement()->MaxAcceleration = 4096.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 3072.f;
	GetCharacterMovement()->AirControl = 0.35f;
}


// 게임 시작 시 한 번 호출: 플레이어 컨트롤러에 입력 매핑 컨텍스트를 등록
void ARabbit::BeginPlay()
{
	Super::BeginPlay();

	// 던전 입장/귀환으로 레벨을 옮겨왔다면 직전 레벨의 상태(HP·스탯·카드)와 도착 위치를 복원
	if (UDarkWolrdGameInstance* GameInstance = Cast<UDarkWolrdGameInstance>(GetGameInstance()))
	{
		GameInstance->RestorePlayerAfterTravel(this);
	}

	// 쓰러졌을 때 돌아올 부활 지점 = 이 레벨에서 처음 선 위치
	RespawnTransform = GetActorTransform();

	// 타격감 연출의 기준값: 지금 시야각·카메라 회전·중력
	if (Camera)
	{
		CurrentBaseFOV = Camera->FieldOfView;
		BaseCameraRotation = Camera->GetRelativeRotation();
	}
	BaseGravityScale = GetCharacterMovement()->GravityScale;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// HUD 위젯 생성: 화면에 계속 떠서 HP/MP/상호작용 프롬프트를 보여줌 (인벤토리처럼 열고 닫는 게 아니라 상시 표시)
		if (HUDWidgetClass && StatComponent)
		{
			HUDWidgetNative = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
			if (HUDWidgetNative)
			{
				HUDWidgetNative->SetStatComponent(StatComponent);
				HUDWidgetNative->SetInteractionComponent(InteractionComponent);
				HUDWidgetNative->AddToViewport();
			}
		}

		// 조준점: 화면 정중앙(평타가 날아가는 곳)과 락온 대상 표시
		if (CrosshairWidgetClass)
		{
			CrosshairWidgetNative = CreateWidget<UCrosshairWidget>(PC, CrosshairWidgetClass);
			if (CrosshairWidgetNative)
			{
				CrosshairWidgetNative->AddToViewport(-1);
			}
		}

		// 대화창 위젯 생성: 평소엔 숨겨져 있다가 Interact() 성공 시에만 보이게 함
		if (DialogueWidgetClass)
		{
			DialogueWidgetNative = CreateWidget<UDialogueWidget>(PC, DialogueWidgetClass);
			if (DialogueWidgetNative)
			{
				DialogueWidgetNative->AddToViewport();
				DialogueWidgetNative->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}


// 매 프레임 호출: 명상 중이면 MP를 초당 MeditateMPPerSecond만큼 회복하고, 입력 없이 지난 시간을 누적
void ARabbit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMeditating && StatComponent)
	{
		StatComponent->RestoreMP(MeditateMPPerSecond * DeltaTime);
	}

	TimeSinceLastInput += DeltaTime;

	UpdateCameraAndFacing(DeltaTime);
	UpdateCameraFeel(GetWorld()->GetDeltaSeconds());

	// 떨어질 때만 중력을 키워서 점프 끝이 둥둥 뜨지 않게 함
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->GravityScale = (MoveComp->IsFalling() && MoveComp->Velocity.Z < 0.f) ? BaseGravityScale * FallGravityMultiplier : BaseGravityScale;

	// 머리 Look At 타겟: 평소에는 카메라가 보는 방향의 먼 지점.
	// 탐험 상태로 달리는 중에는 몸이 달리는 방향을 보므로 고개도 정면(달리는 방향)을 봄.
	// 공격 직후 락온 대상을 보고 있을 때는 고개도 대상을 봄.
	// 방향 사이는 각도로 부드럽게 전환 (점을 직선으로 보간하면 몸을 관통하며 고개가 휙 돌 수 있음)
	if (Camera)
	{
		const FVector Origin = GetActorLocation();
		const bool bStrafe = IsStrafing();
		float LockOnYaw = 0.f;

		FVector DesiredDir = Camera->GetComponentRotation().Vector();
		if (bStrafe && GetLockOnTargetYaw(LockOnYaw))
		{
			DesiredDir = (AbilityComponent->LockedTarget->GetActorLocation() - Origin).GetSafeNormal();
		}
		else if (!bStrafe && GetVelocity().Size2D() > 10.f)
		{
			DesiredDir = GetActorForwardVector();
		}

		FVector CurrentDir = (HeadLookAtTarget - Origin).GetSafeNormal();
		if (CurrentDir.IsNearlyZero())
		{
			CurrentDir = DesiredDir;
		}
		const FVector LookDir = FMath::VInterpNormalRotationTo(CurrentDir, DesiredDir, DeltaTime, 360.f);
		HeadLookAtTarget = Origin + LookDir * HeadLookAtDistance;
	}
}


// 입력 활동 기록: 어떤 입력이든 들어오면 대기 시간을 0으로 초기화
void ARabbit::RegisterInputActivity()
{
	TimeSinceLastInput = 0.f;
}


// 입력 바인딩: 각 입력 액션을 해당 함수에 연결
void ARabbit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction,      ETriggerEvent::Triggered,  this, &ARabbit::Move);
		EIC->BindAction(MoveAction,      ETriggerEvent::Completed,  this, &ARabbit::HandleMoveCompleted);
		EIC->BindAction(LookAction,      ETriggerEvent::Triggered,  this, &ARabbit::Look);
		EIC->BindAction(JumpAction,      ETriggerEvent::Started,    this, &ARabbit::HandleJumpStarted);
		EIC->BindAction(JumpAction,      ETriggerEvent::Completed,  this, &ACharacter::StopJumping);
		EIC->BindAction(InventoryAction, ETriggerEvent::Started,    this, &ARabbit::ToggleInventory);
		EIC->BindAction(AttackAction,    ETriggerEvent::Started,    this, &ARabbit::Attack);
		EIC->BindAction(MeditateAction,  ETriggerEvent::Started,    this, &ARabbit::StartMeditate);
		EIC->BindAction(MeditateAction,  ETriggerEvent::Completed,  this, &ARabbit::StopMeditate);
		EIC->BindAction(LockOnAction,    ETriggerEvent::Started,    this, &ARabbit::ToggleLockOn);
		EIC->BindAction(InteractAction,  ETriggerEvent::Started,    this, &ARabbit::Interact);
		EIC->BindAction(SprintAction,    ETriggerEvent::Started,    this, &ARabbit::StartSprint);
		EIC->BindAction(SprintAction,    ETriggerEvent::Completed,  this, &ARabbit::StopSprint);
		EIC->BindAction(AimAction,       ETriggerEvent::Started,    this, &ARabbit::StartAim);
		EIC->BindAction(AimAction,       ETriggerEvent::Completed,  this, &ARabbit::StopAim);
		EIC->BindAction(SummonToggleAction,    ETriggerEvent::Started, this, &ARabbit::Resummon);
		EIC->BindAction(CommandModifierAction, ETriggerEvent::Started, this, &ARabbit::StartCommandMode);
		EIC->BindAction(Command1Action,        ETriggerEvent::Started, this, &ARabbit::HandleCommand1);
		EIC->BindAction(Command2Action,        ETriggerEvent::Started, this, &ARabbit::HandleCommand2);
	}
}


// 이동 처리: 입력값(앞뒤/좌우)을 카메라 방향 기준으로 변환하여 캐릭터를 움직임.
// 카메라 정면으로 순수하게 이동할 때만 ForwardMoveSpeed, 옆/뒤/대각선 섞이면 NonForwardMoveSpeed로 낮춤
void ARabbit::Move(const FInputActionValue& Value)
{
	RegisterInputActivity();

	if (bIsMeditating || bIsAttackLocked || bIsDashLandLocked) return; // 명상/공격/대시 착지 중에는 이동할 수 없음

	FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller) return;

	// 이동 방향 각도(몸 정면 기준): 0=정면, +90=오른쪽, -90=왼쪽, ±180=뒤. 입력이 없으면(atan2(0,0) 미정의) 마지막 값 유지.
	// 탐험 상태에서는 몸이 이동 방향으로 돌기 때문에 애니메이션 입장에선 항상 "정면으로 달리기"(0).
	// 옆걸음 상태(조준·공격 직후)에서만 실제 각도를 넘겨 옆걸음/뒷걸음이 나오게 함.
	// 몸이 카메라가 아닌 락온 대상을 볼 수도 있으므로 입력을 월드 방향으로 바꾼 뒤 몸 방향과 비교함
	if (!MovementVector.IsNearlyZero())
	{
		const float InputWorldYaw = Controller->GetControlRotation().Yaw + FMath::RadiansToDegrees(FMath::Atan2(MovementVector.X, MovementVector.Y));
		MoveInputAngle = IsStrafing() ? FRotator::NormalizeAxis(InputWorldYaw - GetActorRotation().Yaw) : 0.f;
	}

	// 더블탭 전진 감지: 전진 입력이 "막 시작된 순간"(상승 엣지)이 DoubleTapWindow 이내에 두 번째로 발생하면 대시
	const bool bIsPressingForwardNow = MovementVector.Y > 0.5f;
	if (bIsPressingForwardNow && !bWasPressingForward)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastForwardPressTime <= DoubleTapWindow)
		{
			TryDash();
		}
		LastForwardPressTime = Now;
	}
	bWasPressingForward = bIsPressingForwardNow;

	// 몸 정면으로 가는 중인지: 탐험 상태에서는 MoveInputAngle이 항상 0이라 어느 방향이든 정면 달리기 속도(스프린트 포함).
	// 옆걸음 상태(조준·공격 직후)에서는 몸 정면 쪽으로 갈 때만 빠르고 옆/뒤는 NonForwardMoveSpeed.
	// 소환수 이동속도 패시브 배율은 모든 방향에 공통으로 곱함
	const bool bIsMovingForward = FMath::Abs(MoveInputAngle) < 20.f;
	const float MoveSpeedMultiplier = StatComponent ? StatComponent->GetMoveSpeedMultiplier() : 1.f;
	if (bIsMovingForward)
	{
		GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? SprintSpeed : ForwardMoveSpeed) * MoveSpeedMultiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = NonForwardMoveSpeed * MoveSpeedMultiplier;
	}

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection,   MovementVector.X);
}


// 시점 회전: 마우스 움직임으로 카메라를 상하좌우로 돌림
// 시점 회전은 "입력 활동"으로 치지 않음 — 카메라만 돌려도 대기 애니메이션(BoredIdle)이 풀리면 안 되므로 RegisterInputActivity 호출 안 함
void ARabbit::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller) return;

	AddControllerYawInput(LookAxisVector.X * MouseSensitivity);
	AddControllerPitchInput(-LookAxisVector.Y * MouseSensitivity);
}


// 인벤토리 토글: 닫혀있으면 열고(마우스 커서 활성화), 열려있으면 닫음(게임 조작 복귀)
// 인벤토리 열기/닫기도 "입력 활동"으로 치지 않음 — 인벤토리만 열어봐도 대기 애니메이션이 풀리면 안 되므로 RegisterInputActivity 호출 안 함
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


// 기본공격 입력 처리: AbilityComponent의 공유 캐스팅 슬롯에 "BasicAttack"으로 캐스팅/발사를 위임
void ARabbit::Attack()
{
	RegisterInputActivity();

	// 쿨다운 등으로 실제로 발동되지 않았으면(false 반환) 이동을 잠글 이유가 없음
	if (AbilityComponent && AbilityComponent->TryActivateAbility(TEXT("BasicAttack")))
	{
		// 쏘는 방향으로 즉시 돌아보고 잠시 그 방향을 유지: 락온 중이면 대상 쪽, 아니면 카메라(조준) 방향
		float FacingYaw = 0.f;
		if (GetLockOnTargetYaw(FacingYaw))
		{
			SetActorRotation(FRotator(0.f, FacingYaw, 0.f));
		}
		else if (Controller)
		{
			SetActorRotation(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
		}
		CombatFacingTimeLeft = CombatFacingDuration;
		StartAttackMoveLock();
		if (GameFeelComponent)
		{
			GameFeelComponent->AddSquash(AttackSquash);
		}
		if (FaceComponent)
		{
			FaceComponent->PlayExpression(ERabbitExpression::Angry, 0.6f);
		}
	}
}


// 공격 모션 재생 시간 동안 이동을 잠금. 관성으로 미끄러지지 않게 현재 속도도 즉시 제거
void ARabbit::StartAttackMoveLock()
{
	bIsAttackLocked = true;
	GetCharacterMovement()->StopMovementImmediately();

	// 공격속도가 빠를수록 이동 잠금 시간도 그만큼 짧아짐 (몽타주 재생속도와 동기화)
	const float AttackSpeedMultiplier = StatComponent ? StatComponent->GetAttackSpeedMultiplier() : 1.f;
	const float EffectiveLockDuration = AttackMoveLockDuration / AttackSpeedMultiplier;

	GetWorld()->GetTimerManager().SetTimer(AttackLockTimerHandle, this, &ARabbit::ClearAttackLock, EffectiveLockDuration, false);
}


// 이동 잠금 해제
void ARabbit::ClearAttackLock()
{
	bIsAttackLocked = false;
}


// 점프 입력 처리: 입력 활동을 기록한 뒤 실제 점프 수행 (ACharacter::Jump를 대신 감싸는 래퍼)
void ARabbit::HandleJumpStarted()
{
	RegisterInputActivity();
	Jump();
}


// 락온 토글 입력 처리: AbilityComponent에 위임
void ARabbit::ToggleLockOn()
{
	RegisterInputActivity();

	if (AbilityComponent)
	{
		AbilityComponent->ToggleLockOn();
	}
}


// 상호작용(F) 입력 처리: 상호작용 가능한 NPC가 있으면 대사를 진행시켜 대화창에 표시.
// 상호작용 가능한 대상이 없는데 대화창이 열려있으면, 그 김에 닫아줌(대화 종료 용도)
void ARabbit::Interact()
{
	RegisterInputActivity();

	if (!InteractionComponent || !DialogueWidgetNative) return;

	FText NPCName, Line;
	bool bIsLastLine = false;

	if (InteractionComponent->TryInteract(NPCName, Line, bIsLastLine))
	{
		DialogueWidgetNative->ShowLine(NPCName, Line);
		DialogueWidgetNative->SetVisibility(ESlateVisibility::Visible);
	}
	else if (DialogueWidgetNative->GetVisibility() == ESlateVisibility::Visible)
	{
		DialogueWidgetNative->SetVisibility(ESlateVisibility::Collapsed);
	}
}


// 소환(Y) 입력 처리: 기존 소환수를 취소하고 도감에서 HP가 가득 찬 상태로 바로 재소환
void ARabbit::Resummon()
{
	RegisterInputActivity();

	if (CompanionComponent)
	{
		CompanionComponent->Resummon();
	}
}


// 커맨드 모드 시작(4): CommandModeWindow 동안만 1/2 입력을 소환수 커맨드로 받아들임
void ARabbit::StartCommandMode()
{
	RegisterInputActivity();

	bCommandModeActive = true;
	GetWorldTimerManager().SetTimer(CommandModeTimerHandle, this, &ARabbit::EndCommandMode, CommandModeWindow, false);
}


// 커맨드 모드 해제 (타이머로 자동 호출, 또는 커맨드 실행 후 즉시 호출됨)
void ARabbit::EndCommandMode()
{
	bCommandModeActive = false;
	GetWorldTimerManager().ClearTimer(CommandModeTimerHandle);
}


// "4+1": 모든 소환수를 내 주변에 모이게 함. 커맨드 모드 중이 아니면 무시
void ARabbit::HandleCommand1()
{
	if (!bCommandModeActive) return;
	EndCommandMode();

	if (CompanionComponent)
	{
		CompanionComponent->HandleGatherCommand();
	}
}


// "4+2": 모든 소환수가 내가 락온 중인 대상을 공격하게 함. 커맨드 모드 중이 아니면 무시
void ARabbit::HandleCommand2()
{
	if (!bCommandModeActive) return;
	EndCommandMode();

	if (CompanionComponent)
	{
		CompanionComponent->HandleAttackPlayerTargetCommand();
	}
}


// 스프린트 시작: 다음 Move() 호출부터 순수 전진 시 SprintSpeed 적용
void ARabbit::StartSprint()
{
	RegisterInputActivity();
	bIsSprinting = true;
}


// 스프린트 종료
void ARabbit::StopSprint()
{
	bIsSprinting = false;
}


// 이동 입력이 완전히 끝났을 때(모든 이동 키를 뗐을 때) 호출됨. 더블탭 판정용 상태를 리셋해서
// 다음 전진 입력이 다시 "상승 엣지"로 인식되게 함
void ARabbit::HandleMoveCompleted()
{
	bWasPressingForward = false;
}


// 대시(텔레포트): 캐릭터 정면(카메라 방향 기준)으로 DashDistance만큼 순간이동.
// 벽 등에 막히면 그 앞까지만 이동하고, 순간이동 전후로 화면을 짧게 페이드해 급격한 시점 변화를 완화함
void ARabbit::TryDash()
{
	if (bDashCooldownEnabled && GetWorld()->GetTimeSeconds() < DashCooldownEndTime) return;
	if (!Controller) return;

	// 옆걸음 상태(조준·공격 직후): 카메라 정면으로 대시.
	// 탐험 상태: 지금 누르고 있는 이동 방향(없으면 몸이 보는 방향)으로 대시하고 몸도 그쪽으로 돌림
	FVector DashDirection;
	if (IsStrafing())
	{
		const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
		DashDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	}
	else
	{
		DashDirection = GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
		if (DashDirection.IsNearlyZero())
		{
			DashDirection = GetActorForwardVector().GetSafeNormal2D();
		}
		SetActorRotation(DashDirection.Rotation());
	}

	const FVector StartLocation = GetActorLocation();
	FVector TargetLocation = StartLocation + DashDirection * DashDistance;

	// 벽을 뚫고 순간이동하지 않도록 캐릭터 캡슐 모양 그대로 스윕 검사.
	// 바닥에 붙어있는 캡슐을 그대로 옆으로 스윕하면 바닥 자체를 막힘으로 오인할 수 있어, 살짝 띄워서 검사함
	const FVector TraceOffset(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	const bool bBlocked = GetWorld()->SweepSingleByChannel(
		Hit, StartLocation + TraceOffset, TargetLocation + TraceOffset, FQuat::Identity, ECC_Pawn,
		GetCapsuleComponent()->GetCollisionShape(), QueryParams);

	if (bBlocked)
	{
		TargetLocation = Hit.Location - TraceOffset;
	}

	// 도착 지점을 살짝 위로 띄워서, 중력에 의해 짧게 낙하하며 착지하는 느낌을 줌
	TargetLocation.Z += DashLandHeightOffset;

	// 카메라가 순간이동을 그대로 따라가지 않고, 잠시 느긋하게 뒤쫓아오도록 SpringArm 지연 속도를 낮춤
	if (SpringArm)
	{
		SpringArm->CameraLagSpeed = DashCameraLagSpeed;
		GetWorld()->GetTimerManager().SetTimer(DashCameraLagRestoreTimerHandle, this, &ARabbit::RestoreCameraLagSpeed, DashCameraLagRestoreDelay, false);
	}

	// 착지 시점(Landed())까지 이동을 잠가서, 낙하 중이나 착지 몽타주 재생 중에 걷기 애니메이션이 안 섞이게 함.
	// 혹시 바닥이 없어 Landed()가 안 불리는 경우를 대비해, 최대 대기시간이 지나면 강제로 풀리는 안전장치도 걸어둠
	bIsDashLandLocked = true;
	bIsDashFalling = true;
	GetCharacterMovement()->StopMovementImmediately();
	GetWorld()->GetTimerManager().SetTimer(DashLandLockTimerHandle, this, &ARabbit::ClearDashLandLock, MaxDashFallSafetyDuration, false);

	SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);

	// 순간이동하며 몸이 길쭉해지고 시야가 확 넓어졌다가 돌아옴
	if (GameFeelComponent)
	{
		GameFeelComponent->AddSquash(DashSquash);
	}
	AddFOVKick(DashFOVKick);

	if (bDashCooldownEnabled)
	{
		DashCooldownEndTime = GetWorld()->GetTimeSeconds() + DashCooldown;
	}
}


// 대시로 낮춰뒀던 카메라 지연 속도를 원래대로 되돌림
void ARabbit::RestoreCameraLagSpeed()
{
	if (SpringArm)
	{
		SpringArm->CameraLagSpeed = NormalCameraLagSpeed;
	}
}


// 착지 몽타주 재생 중 걸어둔 이동 잠금 해제
void ARabbit::ClearDashLandLock()
{
	bIsDashLandLocked = false;
}


// 낙하 중 땅에 닿을 때 엔진이 자동으로 호출. 대시로 인한 낙하였으면 이 순간에 착지 몽타주를 재생하고,
// 그 재생 시간만큼 이동 잠금을 유지(타이머로 자동 해제)
void ARabbit::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// 높은 곳에서 떨어졌으면 낙하 속도에 비례해 화면이 쿵 흔들림 (몸 찌그러짐은 GameFeelComponent가 처리)
	if (GameFeelComponent)
	{
		const float LandAlpha = FMath::Clamp((GameFeelComponent->GetLastFallSpeed() - 700.f) / 800.f, 0.f, 1.f);
		AddCameraTrauma(LandCameraTrauma * LandAlpha);
	}

	if (!bIsDashFalling) return;
	bIsDashFalling = false;

	if (DashLandMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(DashLandMontage);
		}
	}

	GetWorld()->GetTimerManager().SetTimer(DashLandLockTimerHandle, this, &ARabbit::ClearDashLandLock, DashLandLockDuration, false);
}


// 명상 시작: 이동을 즉시 멈추고(기존 속도 제거) 명상 상태로 전환. 이후 Tick에서 MP를 회복시킴
void ARabbit::StartMeditate()
{
	RegisterInputActivity();

	bIsMeditating = true;
	GetCharacterMovement()->StopMovementImmediately();
}


// 명상 종료: 다시 이동할 수 있는 상태로 되돌림
void ARabbit::StopMeditate()
{
	bIsMeditating = false;
}


// 디버그: HP를 지정한 양만큼 깎음. HUD의 HP 바가 실제로 반응하는지 콘솔에서 확인하는 용도
void ARabbit::DebugTakeDamage(float Amount)
{
	if (StatComponent)
	{
		StatComponent->ApplyDamage(Amount);
	}
}


// 디버그: MP를 지정한 양만큼 소비. HUD의 MP 바 확인 + 이후 명상으로 회복되는 것까지 같이 테스트 가능
void ARabbit::DebugSpendMP(float Amount)
{
	if (StatComponent)
	{
		StatComponent->SpendMP(Amount);
	}
}


// 디버그 메시지를 화면 왼쪽 위에 띄움 (소환 테스트 명령 공용)
static void ShowSummonDebugMessage(const FString& Message, const FColor& Color = FColor::Cyan)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, Message);
	}
}


// 디버그: 이름이 MonsterBlueprintName인 몬스터 블루프린트를 에셋 레지스트리에서 찾아 카드를 Count장 지급.
// 블루프린트 에셋을 이름으로 찾는 방식이라 에디터(PIE)에서만 동작함
void ARabbit::DebugGiveCard(const FString& MonsterBlueprintName, int32 Count)
{
	if (!CompanionComponent || Count <= 0) return;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);

	TSubclassOf<ABaseMonster> MonsterClass = nullptr;
	for (const FAssetData& Asset : BlueprintAssets)
	{
		if (Asset.AssetName.ToString().Equals(MonsterBlueprintName, ESearchCase::IgnoreCase))
		{
			MonsterClass = LoadClass<ABaseMonster>(nullptr, *(Asset.GetObjectPathString() + TEXT("_C")));
			if (MonsterClass) break;
		}
	}

	if (!MonsterClass)
	{
		ShowSummonDebugMessage(FString::Printf(TEXT("DebugGiveCard: '%s' 이름의 몬스터 블루프린트(BaseMonster 자식)를 찾지 못함"), *MonsterBlueprintName), FColor::Red);
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		FMonsterCardEntry Card;
		Card.MonsterClass = MonsterClass;
		CompanionComponent->AddCard(Card);
	}
	ShowSummonDebugMessage(FString::Printf(TEXT("카드 지급: %s ×%d (보유 %d장)"), *MonsterBlueprintName, Count, CompanionComponent->HeldCards.Num()));
}


// 디버그: 보유 카드 전부 등록
void ARabbit::DebugRegisterAllCards()
{
	if (!CompanionComponent) return;

	const int32 RegisteredCount = CompanionComponent->RegisterAllHeldCards();
	ShowSummonDebugMessage(FString::Printf(TEXT("카드 %d장 등록 → 도감 %d종족"), RegisteredCount, CompanionComponent->RegisteredMonsters.Num()));
}


// 디버그: 소환 관련 상태를 화면에 출력 (AddOnScreenDebugMessage는 나중에 넣은 줄이 위에 표시되므로 역순으로 넣음)
void ARabbit::DebugSummonStatus()
{
	if (!CompanionComponent) return;

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("[소환 상태] 보유 카드 %d장 / 도감 %d종족 / 소환 중 %d마리"),
		CompanionComponent->HeldCards.Num(), CompanionComponent->RegisteredMonsters.Num(), CompanionComponent->ActiveSummons.Num()));

	for (const FRegisteredMonsterEntry& Entry : CompanionComponent->RegisteredMonsters)
	{
		Lines.Add(FString::Printf(TEXT("  도감: %s  등급+%d(×%.2f)  Lv.%d  EXP %d/%d"),
			Entry.MonsterClass ? *Entry.MonsterClass->GetName() : TEXT("(없음)"),
			Entry.UpgradeCount, Entry.GetUpgradeMultiplier(), Entry.Level, Entry.CurrentEXP, Entry.GetEXPToNextLevel()));
	}

	for (const ABaseMonster* Summon : CompanionComponent->ActiveSummons)
	{
		if (IsValid(Summon))
		{
			Lines.Add(FString::Printf(TEXT("  소환수: %s  HP %.0f/%.0f  공격력 %.1f"),
				*Summon->GetClass()->GetName(), Summon->CurrentHP, Summon->GetMaxHP(), Summon->GetAttackPower()));
		}
	}

	for (int32 i = Lines.Num() - 1; i >= 0; --i)
	{
		ShowSummonDebugMessage(Lines[i]);
	}
}


// 피격: 받은 피해를 스탯 HP에 적용하고, 0이 되면 사망 처리. 이미 쓰러진 상태면 무시
float ARabbit::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bIsDead || !StatComponent || ActualDamage <= 0.f) return 0.f;

	StatComponent->ApplyDamage(ActualDamage);

	// 나를 공격한 적을 소환수들이 함께 공격하도록 알림 (투사체면 쏜 쪽 기준)
	if (CompanionComponent && DamageCauser)
	{
		AActor* Attacker = (!DamageCauser->IsA<APawn>() && DamageCauser->GetInstigator()) ? DamageCauser->GetInstigator() : DamageCauser;
		CompanionComponent->NotifyPlayerAttacked(Attacker);
	}

	// 표정: 맞으면 질끈 감고 찡그림
	if (FaceComponent)
	{
		FaceComponent->PlayExpression(ERabbitExpression::Hurt, 0.45f);
	}

	// 타격감: 몸이 움찔 찌그러지며 잠깐 멈추고 화면이 흔들림. 근접 공격이면 때린 몬스터도 같이 멈춤
	if (GameFeelComponent)
	{
		GameFeelComponent->PlayHitReaction(true, StatComponent->CurrentHP <= 0.f);
	}
	AddCameraTrauma(HurtCameraTrauma);
	if (DamageCauser && DamageCauser->IsA<APawn>())
	{
		if (UGameFeelComponent* AttackerFeel = DamageCauser->FindComponentByClass<UGameFeelComponent>())
		{
			AttackerFeel->HitStop(AttackerFeel->HitStopHeavy);
		}
	}

	if (StatComponent->CurrentHP <= 0.f)
	{
		HandleDeath();
	}
	return ActualDamage;
}


// 사망: 입력·이동을 막고, 소환수를 거두고, 락온을 풀고, RespawnDelay 뒤 부활 예약
void ARabbit::HandleDeath()
{
	if (bIsDead) return;
	bIsDead = true;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (CompanionComponent)
	{
		CompanionComponent->DespawnAllSummons();
	}
	if (AbilityComponent)
	{
		AbilityComponent->LockedTarget = nullptr;
	}

	if (FaceComponent)
	{
		FaceComponent->SetBaseExpression(ERabbitExpression::Sad);
	}

	OnDied.Broadcast();

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ARabbit::Respawn, RespawnDelay, false);
}


// 부활: 처음 등장한 위치로 돌아가 HP/MP를 가득 채우고 다시 조작 가능하게 함
void ARabbit::Respawn()
{
	SetActorTransform(RespawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (StatComponent)
	{
		StatComponent->CurrentHP = StatComponent->GetMaxHP();
		StatComponent->CurrentMP = StatComponent->GetMaxMP();
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(RespawnTransform.Rotator());
		EnableInput(PC);
	}

	if (FaceComponent)
	{
		FaceComponent->SetBaseExpression(ERabbitExpression::Neutral);
	}

	bIsDead = false;
}


// 디버그: 레벨의 모든 필드보스 스포너에서 즉시 보스 소환
void ARabbit::DebugSpawnFieldBoss()
{
	int32 SpawnerCount = 0;
	for (TActorIterator<AFieldBossSpawner> It(GetWorld()); It; ++It)
	{
		++SpawnerCount;
		if (!It->SpawnBossNow())
		{
			ShowSummonDebugMessage(FString::Printf(TEXT("%s: 보스 소환 실패 (BossClass 확인)"), *It->GetName()), FColor::Red);
		}
	}
	if (SpawnerCount == 0)
	{
		ShowSummonDebugMessage(TEXT("DebugSpawnFieldBoss: 레벨에 FieldBossSpawner가 없음"), FColor::Red);
	}
}


// 디버그: 반경 안의 적대 몬스터를 전부 쓰러뜨림. 플레이어가 준 피해로 처리되므로 경험치·돈·카드 드랍도 정상 발생
void ARabbit::DebugKillNearby(float Radius)
{
	int32 KilledCount = 0;
	for (TActorIterator<ABaseMonster> It(GetWorld()); It; ++It)
	{
		ABaseMonster* Monster = *It;
		if (!Monster->IsAlive() || !CombatFaction::AreHostile(this, Monster)) continue;
		if (FVector::Dist(Monster->GetActorLocation(), GetActorLocation()) > Radius) continue;

		Monster->TakeDamage(Monster->GetMaxHP() * 10.f, FDamageEvent(), GetController(), this);
		++KilledCount;
	}
	ShowSummonDebugMessage(FString::Printf(TEXT("DebugKillNearby: %d마리 처치"), KilledCount));
}


// 디버그: 레벨의 첫 번째 아레나에서 바로 대전 시작
void ARabbit::DebugStartArena()
{
	for (TActorIterator<AMonsterArena> It(GetWorld()); It; ++It)
	{
		if (!It->StartMatch(this))
		{
			ShowSummonDebugMessage(TEXT("DebugStartArena: 시작 실패 (이미 진행 중이거나 소환수/상대 없음)"), FColor::Red);
		}
		return;
	}
	ShowSummonDebugMessage(TEXT("DebugStartArena: 레벨에 MonsterArena가 없음"), FColor::Red);
}


// 조준 시작/끝: 누르고 있는 동안만 조준 모드
void ARabbit::StartAim()
{
	RegisterInputActivity();
	bIsAiming = true;
}


void ARabbit::StopAim()
{
	bIsAiming = false;
}


// 옆걸음 상태: 조준 중이거나 공격 직후 (락온만 한 상태는 탐험처럼 자유롭게 달림)
bool ARabbit::IsStrafing() const
{
	return bIsAiming || CombatFacingTimeLeft > 0.f;
}


// 락온 대상 방향: 락온 중이고 조준 중이 아니면 토끼에서 대상을 향하는 수평 각도를 채움
bool ARabbit::GetLockOnTargetYaw(float& OutYaw) const
{
	if (bIsAiming || !AbilityComponent || !AbilityComponent->IsLockedOn()) return false;

	OutYaw = (AbilityComponent->LockedTarget->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
	return true;
}


// 매 프레임: 상태에 맞는 몸 회전 방식을 고르고, 카메라 거리·어깨 오프셋·시야각을 목표값으로 부드럽게 이동
void ARabbit::UpdateCameraAndFacing(float DeltaTime)
{
	CombatFacingTimeLeft = FMath::Max(0.f, CombatFacingTimeLeft - DeltaTime);
	const bool bStrafe = IsStrafing();

	// 탐험: 달리는 방향으로 회전 / 옆걸음: 카메라 방향으로 회전, 단 공격 직후 락온 중이면 락온 대상 쪽으로 회전.
	// 모두 회전 속도 제한을 두고 부드럽게 돌도록 bUseControllerRotationYaw(즉시 회전)는 쓰지 않음
	float LockOnYaw = 0.f;
	const bool bFaceLockOnTarget = bStrafe && GetLockOnTargetYaw(LockOnYaw);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = !bStrafe;
	MoveComp->bUseControllerDesiredRotation = bStrafe && !bFaceLockOnTarget;
	MoveComp->RotationRate = FRotator(0.f, bStrafe ? FacingTurnRate : ExploreTurnRate, 0.f);

	if (bFaceLockOnTarget)
	{
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, LockOnYaw, 0.f), DeltaTime, FacingTurnRate));
	}

	if (SpringArm)
	{
		const float TargetLength = bIsAiming ? AimArmLength : ExploreArmLength;
		const FVector TargetOffset = bIsAiming ? AimSocketOffset : ExploreSocketOffset;
		SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetLength, DeltaTime, CameraBlendSpeed);
		SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, TargetOffset, DeltaTime, CameraBlendSpeed);
	}
	if (Camera)
	{
		// 실제 시야각은 UpdateCameraFeel에서 순간 변화량을 더해서 적용
		CurrentBaseFOV = FMath::FInterpTo(CurrentBaseFOV, bIsAiming ? AimFOV : ExploreFOV, DeltaTime, CameraBlendSpeed);
	}
}


void ARabbit::AddCameraTrauma(float Amount)
{
	CameraTrauma = FMath::Clamp(CameraTrauma + Amount, 0.f, 1.f);
}


void ARabbit::AddFOVKick(float Degrees)
{
	FOVKick += Degrees;
}


// 흔들림은 세기의 제곱에 비례해서, 약한 타격은 은은하고 강한 타격만 확 흔들림. 노이즈로 흔들어서 덜 기계적으로 보임
void ARabbit::UpdateCameraFeel(float DeltaTime)
{
	if (!Camera) return;

	UGameFeelComponent::StepSpring(FOVKick, FOVKickVelocity, 0.f, FOVKickFrequency, FOVKickDampingRatio, DeltaTime);
	Camera->SetFieldOfView(CurrentBaseFOV + FOVKick);

	CameraTrauma = FMath::Max(0.f, CameraTrauma - TraumaDecay * DeltaTime);
	ShakeTime += DeltaTime;
	const float ShakeAngle = CameraTrauma * CameraTrauma * MaxShakeAngle;
	const float NoiseTime = ShakeTime * ShakeFrequency;
	const FRotator Shake(
		ShakeAngle * FMath::PerlinNoise1D(NoiseTime),
		ShakeAngle * FMath::PerlinNoise1D(NoiseTime + 37.1f),
		ShakeAngle * 0.5f * FMath::PerlinNoise1D(NoiseTime + 71.3f));
	Camera->SetRelativeRotation(BaseCameraRotation + Shake);
}
