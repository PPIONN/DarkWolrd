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
class UAbilityComponent;
class UCharacterStatComponent;
class UMonsterCompanionComponent;
class UHUDWidget;
class UInteractionComponent;
class UDialogueWidget;

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

	// Attack: 기본공격 입력 처리. AbilityComponent의 "BasicAttack" 슬롯을 캐스팅/발사하고,
	// 실제로 발동됐으면(쿨다운 등으로 무시되지 않았으면) 짧은 시간 이동을 잠금
	void Attack();

	// StartAttackMoveLock: 공격 모션 재생 시간 동안 이동 입력을 무시하도록 잠그고, AttackMoveLockDuration 뒤에 자동 해제
	void StartAttackMoveLock();
	// ClearAttackLock: 이동 잠금 해제 (타이머로 자동 호출됨)
	void ClearAttackLock();

	// StartMeditate: 명상을 시작함. 이동을 즉시 멈추고, 누르고 있는 동안 매 프레임 MP를 회복함
	void StartMeditate();
	// StopMeditate: 명상을 끝내고 다시 이동할 수 있게 함
	void StopMeditate();

	// HandleJumpStarted: 점프 입력을 받으면 입력 활동을 기록한 뒤 실제 점프(ACharacter::Jump)를 수행
	void HandleJumpStarted();

	// ToggleLockOn: Q 입력 처리. AbilityComponent에 락온 토글을 위임
	void ToggleLockOn();

	// Interact: F 입력 처리. InteractionComponent에 상호작용을 위임하고, 성공하면 대화창에 대사를 표시.
	// 상호작용 가능한 대상이 없는데 대화창이 열려있으면, 그걸 닫는 용도로도 씀
	void Interact();

	// Resummon: Y 입력 처리. CompanionComponent에 (재)소환을 위임 - 기존 소환수는 취소되고 HP가 가득 찬 상태로 다시 소환됨
	void Resummon();

	// StartCommandMode: "4" 입력 처리. CommandModeWindow 동안만 다음 1/2 입력을 커맨드로 받아들임
	void StartCommandMode();

	// HandleCommand1: 커맨드 모드 중 "1" 입력 처리 ("내 주변에 모여")
	void HandleCommand1();

	// HandleCommand2: 커맨드 모드 중 "2" 입력 처리 ("내가 공격중인 적을 공격해")
	void HandleCommand2();

	// EndCommandMode: CommandModeWindow가 지나면 자동 호출되어 커맨드 모드를 해제 (타이머)
	void EndCommandMode();

	// StartSprint / StopSprint: Shift를 누르고 있는 동안 스프린트 상태로 전환
	void StartSprint();
	void StopSprint();

	// HandleMoveCompleted: 이동 입력이 완전히 끝났을 때(전부 손 뗐을 때) 호출되어 더블탭 감지용 상태를 리셋
	void HandleMoveCompleted();

	// TryDash: 쿨다운(활성화된 경우)이 끝났으면 캐릭터 정면으로 DashDistance만큼 순간이동
	void TryDash();

	// RestoreCameraLagSpeed: 대시로 낮춰뒀던 카메라 지연 속도를 원래대로 되돌림 (타이머로 지연 호출됨)
	void RestoreCameraLagSpeed();

	// ClearDashLandLock: 착지 몽타주 재생 중 걸어둔 이동 잠금을 해제 (타이머로 지연 호출됨)
	void ClearDashLandLock();

	// RegisterInputActivity: TimeSinceLastInput을 0으로 리셋.
	// 이동/시점/공격/명상/인벤토리/점프 등 입력 처리 함수 맨 앞에서 호출되며,
	// BlueprintCallable이라 애님 블루프린트에서도 호출 가능(예: BoredIdle 재생이 끝났을 때 다음 10초 카운트다운을 다시 시작시키는 용도)
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void RegisterInputActivity();

public:
	// GetTimeSinceLastInput: 마지막 입력 이후 지난 시간(초). 소환수 AI가 "플레이어가 가만히 있는지" 판단할 때 사용
	float GetTimeSinceLastInput() const { return TimeSinceLastInput; }

	// ─── 디버그/테스트용 ───────────────────────────────────────
	// PIE 중 `(콘솔 열기) 키를 누르고 "DebugTakeDamage 20" 처럼 입력해서 HUD의 HP/MP 바가
	// 실제로 반응하는지 확인하는 용도. Exec로 선언된 함수는 콘솔에서 함수 이름 그대로 호출 가능
	UFUNCTION(Exec)
	void DebugTakeDamage(float Amount = 20.f);

	UFUNCTION(Exec)
	void DebugSpendMP(float Amount = 20.f);

	// ─── 소환 시스템 테스트용 콘솔 명령 ───────────────────────────────────────
	// DebugGiveCard BP_Bat 3 → 이름이 BP_Bat인 몬스터 블루프린트 카드를 3장 보유 목록에 추가 (에디터 PIE 전용)
	UFUNCTION(Exec)
	void DebugGiveCard(const FString& MonsterBlueprintName, int32 Count = 1);

	// DebugRegisterAllCards → 보유 카드를 전부 도감에 등록 (신규 종족은 등록, 이미 있는 종족은 등급 +1)
	UFUNCTION(Exec)
	void DebugRegisterAllCards();

	// DebugSummonStatus → 보유 카드 수, 도감(등급/레벨/경험치), 현재 소환수 HP를 화면에 출력
	UFUNCTION(Exec)
	void DebugSummonStatus();

	// Landed: 캐릭터가 낙하 중 땅에 닿을 때 엔진이 자동으로 호출. 대시로 인한 낙하였으면 여기서 착지 몽타주를 재생함
	virtual void Landed(const FHitResult& Hit) override;


	// Tick: 매 프레임마다 자동으로 호출되는 함수. DeltaTime = 이전 프레임과의 시간 간격(초)
	virtual void Tick(float DeltaTime) override;
	// SetupPlayerInputComponent: 키보드·마우스 입력 액션을 각 함수에 연결하는 함수
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// UPROPERTY: 이 변수를 언리얼 에디터에서 확인·관리할 수 있게 해주는 매크로
	// EditAnywhere: 에디터(BP 뷰포트 포함)에서 직접 위치·회전 등을 수정할 수 있음
	// SpringArm: 카메라를 캐릭터 등 뒤에 띄워주는 팔. 벽에 닿으면 카메라를 자동으로 당겨줌
	UPROPERTY(EditAnywhere, Category = "Camera")
	USpringArmComponent* SpringArm;

	// Camera: 플레이어 시점을 담당하는 카메라 컴포넌트
	UPROPERTY(EditAnywhere, Category = "Camera")
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

	// MouseSensitivity: 마우스 시점 회전 속도 배율 (값이 작을수록 천천히 회전함, 기본값 1.0)
	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float MouseSensitivity = 1.0f;

	// JumpAction: 점프 입력(스페이스바 등)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	// InventoryAction: 인벤토리 열기/닫기 입력과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InventoryAction;

	// AttackAction: 기본공격 입력(마우스 좌클릭 등)과 연결된 입력 액션 에셋
	// bInstantCast 어빌리티라 누르는 즉시 오브가 생성되어 곧바로 발사됨
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AttackAction;

	// LockOnAction: 락온 토글 입력(Q 등)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LockOnAction;

	// InteractAction: NPC 상호작용(대화) 입력(F 등)과 연결된 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	// SummonToggleAction: (재)소환 입력(Y)과 연결된 입력 액션 에셋 (BP에 이미 지정된 값이 유지되도록 이름은 그대로 둠)
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SummonToggleAction;

	// CommandModifierAction: 소환수 커맨드 입력(4)과 연결된 입력 액션 에셋.
	// 이 키를 누른 뒤 CommandModeWindow 안에 1/2를 눌러야 커맨드가 실행됨 (팰월드 방식)
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CommandModifierAction;

	// Command1Action / Command2Action: 커맨드 모드 중 눌러야 하는 1(모여) / 2(내 타겟 공격) 입력 액션 에셋
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Command1Action;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* Command2Action;

	// CommandModeWindow: "4"를 누른 뒤 이 시간(초) 안에 1/2를 눌러야 커맨드로 인정됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float CommandModeWindow = 2.f;

	// MeditateAction: 명상(마나 회복) 입력(마우스 우클릭 등)과 연결된 입력 액션 에셋.
	// 누르고 있는 동안 MP를 회복하되, 명상 중에는 이동할 수 없음(기를 모으듯 가만히 서있어야 함)
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MeditateAction;

	// MeditateMPPerSecond: 명상 중 초당 회복되는 MP량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float MeditateMPPerSecond = 10.f;

	// AttackMoveLockDuration: 기본공격 시전 시 이동이 잠기는 시간(초). 공격 몽타주 재생 시간에 맞춰 공격속도 느낌을 냄
	// (기본값 2.67은 현재 기본공격 몽타주 길이에 맞춘 것 — 몽타주가 바뀌면 이 값도 같이 조정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float AttackMoveLockDuration = 2.67f;

	// ForwardMoveSpeed: 카메라가 보는 방향으로 순수하게(옆/뒤 섞임 없이) 이동할 때의 기본(걷기) 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float ForwardMoveSpeed = 300.f;

	// NonForwardMoveSpeed: 옆걸음/뒷걸음/대각선 등 카메라 정면이 아닌 방향으로 이동할 때의 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float NonForwardMoveSpeed = 300.f;

	// SprintSpeed: Shift를 누른 채 순수 전진할 때의 속도. 옆/뒤로는 스프린트 적용 안 됨(NonForwardMoveSpeed 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 600.f;

	// SprintAction: 달리기(스프린트) 입력(Shift 등)과 연결된 입력 액션 에셋. 누르고 있는 동안만 적용
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

	// ─── 대시(텔레포트) ───────────────────────────────────────
	// 마법사 컨셉이라 빠른 대시보다 순간이동으로 구현. 전진(W)을 짧은 시간 안에 두 번 누르면 발동

	// DashDistance: 텔레포트 이동 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashDistance = 500.f;

	// DashLandHeightOffset: 텔레포트 도착 지점을 이 높이만큼 위로 띄워서, 중력에 의해 짧게 낙하하며 착지하는 느낌을 줌
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashLandHeightOffset = 50.f;

	// DoubleTapWindow: 이 시간(초) 이내에 전진 키를 다시 누르면 더블탭으로 인식
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DoubleTapWindow = 0.3f;

	// bDashCooldownEnabled: false인 동안은 쿨다운 체크 자체를 안 함(테스트 편의용). 밸런스 확정되면 true로 전환
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	bool bDashCooldownEnabled = false;

	// DashCooldown: 대시 재사용 대기시간(초). bDashCooldownEnabled가 true일 때만 실제로 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown = 3.f;

	// NormalCameraLagSpeed: 평소 카메라가 캐릭터를 따라가는 속도 (SpringArm의 CameraLagSpeed 기본값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float NormalCameraLagSpeed = 10.f;

	// DashCameraLagSpeed: 대시 직후 잠깐 이 값으로 낮춰서, 캐릭터가 먼저 훅 이동하고 카메라가 여유 있게 뒤쫓아오는 느낌을 냄
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCameraLagSpeed = 2.f;

	// DashCameraLagRestoreDelay: 대시 후 이 시간(초)이 지나면 카메라 지연 속도를 NormalCameraLagSpeed로 되돌림
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCameraLagRestoreDelay = 0.4f;

	// DashLandMontage: 텔레포트 착지 순간 재생할 몽타주.
	// 착지 충격파 VFX는 이 몽타주 자체에 "Play Niagara Particle System" Notify로 붙이면 됨(별도 C++ 처리 불필요)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	UAnimMontage* DashLandMontage;

	// DashLandLockDuration: 착지 몽타주 재생 중 이동을 잠그는 시간(초). 몽타주 재생 중 이동 입력이 겹쳐서
	// 걷기 애니메이션에 착지 모션이 묻히지 않도록, 몽타주 길이에 맞춰 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashLandLockDuration = 0.5f;

	// MaxDashFallSafetyDuration: 대시 후 이 시간(초) 안에 Landed()가 호출되지 않으면(바닥 없는 구덩이 등)
	// 강제로 이동 잠금을 풀어주는 안전장치. 정상적으로 착지하면 이 값 대신 DashLandLockDuration이 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float MaxDashFallSafetyDuration = 3.f;

	// AbilityComponent: 캐스팅 슬롯·오브 스폰/발사를 담당하는 컴포넌트.
	// 기본공격("BasicAttack")과 앞으로 추가할 스킬들이 전부 이 컴포넌트의 Abilities 맵에 등록되어 동작한다
	UPROPERTY(VisibleAnywhere, Category = "Ability")
	UAbilityComponent* AbilityComponent;

	// StatComponent: 레벨/HP/MP/마력/지능/체력/방어력/소지금을 관리하는 스탯 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Stats")
	UCharacterStatComponent* StatComponent;

	// CompanionComponent: 몬스터 카드 습득/등록(도감)을 관리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Monster Card")
	UMonsterCompanionComponent* CompanionComponent;

	// InteractionComponent: NPC 상호작용(범위+조준 판정, 대화 진행)을 담당하는 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	UInteractionComponent* InteractionComponent;

	// TSubclassOf<UUserWidget>: UUserWidget을 상속한 클래스(주로 블루프린트 UI)만 받는 타입
	// InventoryWidgetClass: 에디터에서 지정할 인벤토리 UI의 블루프린트 클래스
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	// InventoryWidgetNative: 실제로 화면에 생성된 인벤토리 위젯 객체 (없으면 nullptr)
	UPROPERTY()
	UUserWidget* InventoryWidgetNative;

	// HUDWidgetClass: 에디터에서 지정할 HP/MP HUD 위젯 블루프린트 (UHUDWidget을 부모로 만든 위젯)
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	// HUDWidgetNative: 게임 시작 시 자동으로 생성되어 화면에 계속 떠 있는 HUD 위젯 (없으면 nullptr)
	UPROPERTY()
	UHUDWidget* HUDWidgetNative;

	// DialogueWidgetClass: 에디터에서 지정할 대화창 위젯 블루프린트 (UDialogueWidget을 부모로 만든 위젯)
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UDialogueWidget> DialogueWidgetClass;

	// DialogueWidgetNative: 게임 시작 시 자동으로 생성되어 있는(평소엔 숨겨진) 대화창 위젯 (없으면 nullptr)
	UPROPERTY()
	UDialogueWidget* DialogueWidgetNative;

	// TimeSinceLastInput: 가장 최근 입력(이동/시점/공격/명상/인벤토리/점프) 이후 지난 시간(초).
	// 애님 블루프린트에서 이 값을 읽어 "일정 시간 이상 입력이 없으면" 지루해하는 대기 애니메이션으로 전환하는 데 사용
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float TimeSinceLastInput = 0.f;

	// MoveInputAngle: 방향키(또는 조이스틱) 입력 자체의 각도(도, -180~180).
	// 0=정면, +90=오른쪽, -90=왼쪽, ±180=뒤. 속도가 아니라 입력을 그대로 쓰기 때문에 지연 없이 즉각 반응함.
	// 스트레이프 블렌드 스페이스의 Direction 값으로 사용하면 됨. 입력이 없을 땐 마지막 값을 그대로 유지
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float MoveInputAngle = 0.f;

	// HeadLookAtTarget: 카메라가 바라보는 먼 지점의 월드 좌표. 매 프레임 갱신됨.
	// 애님 블루프린트에서 머리/목 뼈의 "Look At" 본 컨트롤 타겟으로 사용하면, 캐릭터가 카메라 보는 방향을 따라 고개를 돌림
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	FVector HeadLookAtTarget;

	// HeadLookAtDistance: HeadLookAtTarget을 계산할 때 카메라 앞으로 얼마나 먼 지점을 볼지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float HeadLookAtDistance = 1000.f;

private:
	// bIsMeditating: 명상 중인지 여부. true면 Move() 입력을 무시해 이동이 불가능해짐
	bool bIsMeditating = false;

	// bIsAttackLocked: 공격 모션 재생 중인지 여부. true면 Move() 입력을 무시해 이동이 불가능해짐
	bool bIsAttackLocked = false;

	// bIsDashLandLocked: 대시 착지 몽타주 재생 중인지 여부. true면 Move() 입력을 무시해 이동이 불가능해짐
	bool bIsDashLandLocked = false;

	// bIsDashFalling: 대시로 인해 지금 공중에서 낙하 중인지 여부. Landed()에서 이 값을 보고
	// "일반 낙하"와 "대시 낙하"를 구분해서, 대시 낙하일 때만 착지 몽타주를 재생함
	bool bIsDashFalling = false;

	FTimerHandle DashLandLockTimerHandle;

	// bIsSprinting: Shift를 누르고 있는지 여부. 순수 전진 중일 때만 SprintSpeed를 적용하는 데 사용
	bool bIsSprinting = false;

	// bWasPressingForward / LastForwardPressTime: 전진 키의 상승 엣지를 감지해서 더블탭 대시를 판정하는 데 사용
	bool bWasPressingForward = false;
	float LastForwardPressTime = -100.f;

	// DashCooldownEndTime: bDashCooldownEnabled가 true일 때, 이 월드 시간이 지나야 다시 대시 가능
	float DashCooldownEndTime = -100.f;

	FTimerHandle DashCameraLagRestoreTimerHandle;

	FTimerHandle AttackLockTimerHandle;

	// bCommandModeActive: "4"를 누른 뒤 CommandModeWindow가 지나기 전까지 true.
	// 이 동안에만 1/2 입력이 소환수 커맨드로 처리됨
	bool bCommandModeActive = false;

	FTimerHandle CommandModeTimerHandle;
};
