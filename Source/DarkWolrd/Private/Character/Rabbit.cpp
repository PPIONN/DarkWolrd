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
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "UI/HUDWidget.h"
#include "UI/DialogueWidget.h"
#include "Interaction/InteractionComponent.h"
#include "NPC/NPCBase.h"
#include "Components/CapsuleComponent.h"


// 생성자: 스프링암·카메라 컴포넌트를 만들고 캐릭터 이동 방식을 설정
ARabbit::ARabbit()
{
	PrimaryActorTick.bCanEverTick = true;

	// SpringArm 설정
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->bUsePawnControlRotation = true;

	// 카메라 지연 추적: 캐릭터가 갑자기 순간이동해도 카메라가 그 자리에서 즉시 스냅되지 않고 부드럽게 뒤쫓아가게 함
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = NormalCameraLagSpeed;

	// Camera 설정
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// 캐릭터가 항상 카메라(컨트롤러) 방향을 보도록 함 — 스트레이프(옆걸음) 애니메이션이 실제로 트리거되려면
	// 이동 방향으로 몸이 따라 도는 게 아니라, 몸은 카메라 방향을 유지한 채 이동 방향만 달라져야 함
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));
	StatComponent = CreateDefaultSubobject<UCharacterStatComponent>(TEXT("StatComponent"));
	CompanionComponent = CreateDefaultSubobject<UMonsterCompanionComponent>(TEXT("CompanionComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
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

	// 머리 Look At 타겟: 카메라가 보는 방향으로 먼 지점을 계산해서 ABP에서 머리/목 본 컨트롤에 쓸 수 있게 함
	if (Camera)
	{
		HeadLookAtTarget = Camera->GetComponentLocation() + Camera->GetComponentRotation().Vector() * HeadLookAtDistance;
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

	// 입력 각도 계산: 0=정면, +90=오른쪽, -90=왼쪽, ±180=뒤. 입력이 없으면(atan2(0,0) 미정의) 마지막 값 유지
	if (!MovementVector.IsNearlyZero())
	{
		MoveInputAngle = FMath::RadiansToDegrees(FMath::Atan2(MovementVector.X, MovementVector.Y));
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

	// 순수 전진 여부: 앞으로 가는 성분이 있고, 옆으로 가는 성분이 거의 없어야 함
	const bool bIsPureForward = MovementVector.Y > 0.f && FMath::IsNearlyZero(MovementVector.X, 0.1f);

	// 스프린트는 순수 전진일 때만 적용됨 (옆/뒤로는 항상 NonForwardMoveSpeed 유지).
	// 소환수 이동속도 패시브 배율은 모든 방향에 공통으로 곱함
	const float MoveSpeedMultiplier = StatComponent ? StatComponent->GetMoveSpeedMultiplier() : 1.f;
	if (bIsPureForward)
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
		StartAttackMoveLock();
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

	const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
	const FVector DashDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

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
