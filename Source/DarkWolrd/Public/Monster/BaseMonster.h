// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// ACharacter: 이동, 점프, 충돌 등 기본 기능이 내장된 캐릭터 기본 클래스
#include "GameFramework/Character.h"
#include "Summon/SummonTypes.h"
#include "Combat/CombatFaction.h"
#include "BaseMonster.generated.h"

class UNiagaraSystem;
class USoundBase;
class UNiagaraComponent;
class USummonAIComponent;
class UWildMonsterAIComponent;
class UAnimMontage;
class UAnimSequence;
class AMonsterCardPickup;
class UWidgetComponent;
class UGameFeelComponent;

// EMonsterGrade: 몬스터 등급. 전투 스탯과는 무관하며, 이 몬스터가 드랍할 수 있는
// 장비/무기/악세서리의 최고 등급만 결정한다 (장비 등급 체계와 동일).
UENUM(BlueprintType)
enum class EMonsterGrade : uint8
{
	Basic,		// 일반 몬스터
	Rare,		// 보스 몬스터
	Unique,		// 히든 몬스터
	Legendary	// 최종 보스
};

class ABaseMonster;

// FOnMonsterDied: 몬스터가 쓰러지는 순간 브로드캐스트 (필드보스 스포너, 던전 웨이브 집계 등에서 구독)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterDied, ABaseMonster*, DeadMonster);

// ABaseMonster: 모든 몬스터의 부모(베이스) 클래스
// 새 몬스터를 만들 때 이 클래스를 상속받아 사용하면 됨
// 'A'는 Actor(게임 세상에 존재하는 물체)를 의미
UCLASS()
class DARKWOLRD_API ABaseMonster : public ACharacter
{
	// GENERATED_BODY: 언리얼 엔진이 자동으로 필요한 코드를 생성해주는 매크로
	GENERATED_BODY()

public:
	// 생성자: 몬스터가 처음 만들어질 때 기본값을 설정
	ABaseMonster();

protected:
	// BeginPlay: 몬스터가 게임 세상에 등장할 때 한 번 호출되는 함수
	virtual void BeginPlay() override;

public:
	// Tick: 매 프레임마다 자동으로 호출되는 함수
	virtual void Tick(float DeltaTime) override;

	// ─── 몬스터 기본 스탯 ───────────────────────────────────────

	// EditAnywhere: 에디터와 블루프린트에서 값을 직접 수정 가능
	// BlueprintReadWrite: 블루프린트에서 이 변수를 읽고 쓸 수 있음
	// Category: 에디터에서 표시될 분류 탭 이름

	// Level: 몬스터의 레벨. 레벨에 따라 스탯이 달라지도록 확장 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	int32 Level = 1;

	// Grade: 드랍 아이템 등급을 결정하는 몬스터 등급 (전투 스탯과는 무관)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	EMonsterGrade Grade = EMonsterGrade::Basic;

	// bIgnoresPlayerCharacter: 켜면 플레이어 캐릭터(토끼)를 공격 대상으로 삼지 않고, 토끼가 직접 주는 피해도 받지 않음.
	// 소환수끼리만 싸우는 몬스터대전 상대용 (플레이어 소환수가 주는 피해는 정상으로 받음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	bool bIgnoresPlayerCharacter = false;

	// Faction: 전투 진영. 야생 몬스터는 Wild, 플레이어 소환수는 소환될 때 Player로 바뀜, 몬스터대전 상대는 ArenaOpponent.
	// 서로 다른 진영끼리만 싸움
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	ECombatFaction Faction = ECombatFaction::Wild;

	// BaseMaxHP: 1레벨 기준 기본 최대 HP (임시로 플레이어 Rabbit과 동일한 수치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float BaseMaxHP = 50.f;

	// HPPerLevel: 레벨업 1회당 늘어나는 HP (임시로 플레이어 Rabbit과 동일한 수치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float HPPerLevel = 10.f;

	// CurrentHP: 몬스터의 현재 체력. 0이 되면 사망 처리
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Stats")
	float CurrentHP;

	// BaseAttackPower: 1레벨 기준 기본 공격력 (임시로 플레이어 Rabbit의 기본 마력과 동일한 수치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float BaseAttackPower = 5.f;

	// AttackPowerPerLevel: 레벨업 1회당 늘어나는 공격력.
	// 플레이어는 마력이 레벨업으로 늘지 않아 대응되는 수치가 없으므로, 임시로 HP 증가폭을 재사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float AttackPowerPerLevel = 10.f;

	// Defense: 몬스터의 방어력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float Defense = 0.f;

	// BaseDamagePercentOfPlayerMaxHP: 플레이어 공격 시 기준이 되는, 플레이어 최대 HP 대비 기본 피해 비율.
	// 모든 몬스터 공통값이며(기본 5%), 몬스터 간 강함 차이는 AttackPower로 표현한다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float BaseDamagePercentOfPlayerMaxHP = 0.05f;

	// RewardEXP: 몬스터를 처치했을 때 플레이어가 얻는 경험치 (소환수가 처치해도 플레이어가 받고, 그 소환수 종족도 같은 양을 받음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Stats")
	float RewardEXP = 20.f;

	// AttackPowerBuffMultiplier: 일시적인 공격력 배율 (보스 페이즈 강화 등). 기본 1
	UPROPERTY(BlueprintReadWrite, Category = "Monster|Stats")
	float AttackPowerBuffMultiplier = 1.f;

	// OnMonsterDied: 쓰러지는 순간 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Monster|Stats")
	FOnMonsterDied OnMonsterDied;

	// CardDropChance: 처치 시 이 몬스터의 카드를 드랍할 확률 (0~1). 모든 등급에서 드랍 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Card")
	float CardDropChance = 0.15f;

	// CardPickupClass: 드랍할 카드 액터. 비워두면 기본 카드(회전하는 흰 판). 모양을 바꾸려면 MonsterCardPickup을 부모로 BP를 만들어 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Card")
	TSubclassOf<AMonsterCardPickup> CardPickupClass;

	// MonsterName: 화면에 표시할 이름 (카드 획득·도감 등록 알림 등). 비워두면 블루프린트 이름을 씀
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Card")
	FText MonsterName;

	// GetMonsterDisplayName: MonsterName, 비어있으면 클래스 이름(BP_ 접두사와 _C 접미사 제거)
	UFUNCTION(BlueprintCallable, Category = "Monster|Card")
	FText GetMonsterDisplayName() const;

	// ─── 피격 / 사망 연출 (야생 몬스터·소환수 공통) ───────────────────────────────────────

	// HitMontage: 피해를 받고 살아남았을 때 재생할 몽타주. 공격 등 다른 몽타주가 재생 중이면 끊지 않음.
	// 애님 BP의 AnimGraph에 Slot 'DefaultSlot' 노드가 있어야 보임
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	UAnimMontage* HitMontage = nullptr;

	// DeathAnimation: 사망 시 한 번 재생하고 마지막 포즈로 멈춰 있을 애니메이션 시퀀스 (몽타주로 만들 필요 없음).
	// 비워두면 예전처럼 죽는 즉시 사라짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	UAnimSequence* DeathAnimation = nullptr;

	// CorpseLingerTime: 사망 애니메이션이 끝난 뒤 시체가 남아 있는 시간(초). 그 뒤 사라짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Animation")
	float CorpseLingerTime = 2.f;

	// ─── 소환수 상태 (플레이어가 소환한 개체일 때만 사용) ───────────────────────────────────────

	// bIsPlayerSummon: 이 개체가 플레이어의 소환수인지. true가 되는 순간 SummonAIComponent가 활성화됨
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Summon")
	bool bIsPlayerSummon = false;

	// SummonPowerMultiplier: 소환수로 스폰될 때 카드 중첩 등록(등급)에 따라 적용되는 스탯 배율.
	// 야생 몬스터는 항상 1(영향 없음)
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Summon")
	float SummonPowerMultiplier = 1.f;

	// SummonAIComponent: 소환수 반자율 전투 AI. 모든 몬스터가 기본으로 가지고 있지만
	// bIsPlayerSummon이 true로 Activate()되기 전까지는 아무 동작도 하지 않음
	UPROPERTY(VisibleAnywhere, Category = "Monster|Summon")
	USummonAIComponent* SummonAIComponent;

	// WildAIComponent: 야생 몬스터 AI (감지·추격·귀환·순찰). 게임 시작 시 자동으로 켜지고, 소환수로 스폰된 개체에서는 꺼져 있음
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	UWildMonsterAIComponent* WildAIComponent;

	// GameFeelComponent: 피격 시 찌그러짐·히트스톱, 이동 기울기, 소환 등장 크기 연출
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Game Feel")
	UGameFeelComponent* GameFeelComponent;

	// ─── 머리 위 HP바 / 피해 숫자 ───────────────────────────────────────

	// HPBarComponent: 머리 위 HP바. 화면에 고정 크기로 그려짐(Screen 공간). 크기는 Draw Size, 모양은 Widget Class로 바꿀 수 있음
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|UI")
	UWidgetComponent* HPBarComponent;

	// bAlwaysShowHPBar: 켜면 항상 표시. 끄면 피해를 받은 뒤 HPBarHideDelay초 동안만 표시 (플레이어 소환수는 항상 표시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	bool bAlwaysShowHPBar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	float HPBarHideDelay = 5.f;

	// HPBarDeathLingerTime: 쓰러진 뒤 HP바를 이 시간(초) 동안 더 보여줌 (한 방에 쓰러져도 바가 비는 모습이 보이게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	float HPBarDeathLingerTime = 1.f;

	// HPBarHeightOffset: 캡슐 꼭대기에서 HP바를 얼마나 더 위에 띄울지(cm). 날개·귀 등이 캡슐 위로 튀어나온 몬스터는 키우면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	float HPBarHeightOffset = 30.f;

	// EnemyHPBarColor / SummonHPBarColor: 적(야생·대전 상대)과 내 소환수의 HP바 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	FLinearColor EnemyHPBarColor = FLinearColor(0.85f, 0.12f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	FLinearColor SummonHPBarColor = FLinearColor(0.25f, 0.8f, 0.3f);

	// bShowDamageNumbers: 피해를 받을 때 숫자를 띄울지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|UI")
	bool bShowDamageNumbers = true;

	// SetHPBarVisible: HP바를 보이거나 숨김 (소환 연출로 숨겨둔 소환수 등)
	void SetHPBarVisible(bool bVisible);

	// ─── 소환수 자동 전투 / 파트너 패시브 (몬스터 BP마다 설정) ───────────────────────────────────────

	// BasicAttackMontage: 소환수로 싸울 때 평타 몽타주 (없으면 애니메이션 없이 피해만 들어감)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|Combat")
	UAnimMontage* BasicAttackMontage = nullptr;

	// BasicAttackHitDelay: 평타 시작 후 실제로 피해가 들어가는 시점(초). 몽타주의 타격 프레임에 맞춰 조절
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|Combat")
	float BasicAttackHitDelay = 0.3f;

	// ActiveSkills: 소환수로 싸울 때 쿨타임마다 자동으로 쓰는 스킬. 배열 앞쪽일수록 우선 (팰월드처럼 3개 정도 권장)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|Combat")
	TArray<FSummonActiveSkill> ActiveSkills;

	// PartnerPassiveType / PartnerPassiveValue: 이 몬스터가 소환되어 있는 동안 플레이어가 받는 패시브.
	// 카드 중첩 등록 배율(GetUpgradeMultiplier)만큼 강해지고, 소환 해제·사망 시 사라짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|Passive")
	ESummonPassiveType PartnerPassiveType = ESummonPassiveType::None;

	// PartnerPassiveValue: HP/MP/방어력/재생은 고정 수치(+20 등), 공격속도/이동속도는 비율(0.1 = +10%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|Passive")
	float PartnerPassiveValue = 0.f;

	// SummonVFX / SummonSound: 이 몬스터가 소환될 때 발밑에서 재생할 이펙트·사운드.
	// 비워두면 플레이어의 MonsterCompanion 컴포넌트에 지정된 공통 연출을 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|FX")
	UNiagaraSystem* SummonVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Summon|FX")
	USoundBase* SummonSound = nullptr;

	// HitSound / DeathSound: 맞았을 때 / 쓰러질 때 재생할 소리. 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|FX")
	USoundBase* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|FX")
	USoundBase* DeathSound = nullptr;

	// DeathVFX: 쓰러지는 자리에서 재생할 이펙트. 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|FX")
	UNiagaraSystem* DeathVFX = nullptr;

	// CardDropSound: 카드가 떨어질 때 재생할 소리. 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|FX")
	USoundBase* CardDropSound = nullptr;

	// PlaySummonAppear: 몸(메시)이 아주 작은 크기에서 살짝 커졌다가 원래 크기로 돌아오며 튀어나오는 연출 (Duration초)
	void PlaySummonAppear(float Duration);

	// ─── 저주 상태 ("저주하는 자" 직업 스킬이 거는 디버프) ───────────────────────────────────────
	// 저주에 걸리면 몸 전체에 보라색 VFX가 씌워지고, 걸려있는 동안 받는 모든 대미지가 증폭된다.
	// 중첩되지 않으며, 이미 걸린 상태에서 다시 걸리면 배율은 최신 값으로 덮어쓰고 지속시간만 새로고침한다.

	// CurseVFX: 저주 상태 동안 몸에 씌워지는 보라색 나이아가라 이펙트 (에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Curse")
	UNiagaraSystem* CurseVFX;

	// CurseDamageAmplifyMultiplier: 저주 상태일 때 받는 모든 대미지에 곱해지는 배율. 저주가 아니면 1
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Curse")
	float CurseDamageAmplifyMultiplier = 1.f;

	// IsCursed: 현재 저주 상태인지
	UFUNCTION(BlueprintCallable, Category = "Monster|Curse")
	bool IsCursed() const { return CurseDamageAmplifyMultiplier > 1.f; }

	// ApplyCurse: 저주를 겁니다(또는 갱신). AmplifyPercent = 대미지 증폭 비율(30 = +30%), Duration = 지속시간(초).
	// 이미 걸려있어도 중첩되지 않고 배율은 최신 값으로, 지속시간은 새로고침됨
	UFUNCTION(BlueprintCallable, Category = "Monster|Curse")
	void ApplyCurse(float AmplifyPercent, float Duration);

	// ─── 몬스터 기본 함수 ───────────────────────────────────────

	// GetMaxHP: 현재 레벨을 반영한 최대 HP
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	float GetMaxHP() const;

	// GetAttackPower: 현재 레벨을 반영한 공격력
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	float GetAttackPower() const;

	// CalculatePlayerDamage: 플레이어의 최대 HP와 방어력을 받아 이 몬스터가 플레이어에게 주는 피해량을 계산.
	// 공식: (플레이어 최대 HP × BaseDamagePercentOfPlayerMaxHP) + 몬스터 공격력 - (플레이어 방어력 × 0.5), 최소 1
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	float CalculatePlayerDamage(float PlayerMaxHP, float PlayerDefense) const;

	// GetMoneyDropAmount: 처치 시 드랍할 돈을 계산. 10 + (레벨×5) + 0~20 랜덤
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	int32 GetMoneyDropAmount() const;

	// TryDropMonsterCard: CardDropChance 확률로 이 몬스터의 카드를 죽은 위치에 스폰
	void TryDropMonsterCard();

	// TakeDamage: 몬스터가 데미지를 받을 때 호출되는 함수. 반환값은 실제로 받은 데미지
	// 언리얼 엔진 기본 데미지 시스템을 오버라이드(재정의)함
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// Die: 몬스터가 사망했을 때 호출되는 함수. 자식 클래스에서 재정의하여 사망 연출 추가 가능
	virtual void Die();

	// IsAlive: 몬스터가 살아있는지 확인하는 함수. true면 살아있음
	UFUNCTION(BlueprintCallable, Category = "Monster|Stats")
	bool IsAlive() const;

private:
	// ClearCurse: 저주 지속시간이 끝났을 때 호출되어 배율을 원래대로 되돌리고 VFX를 제거
	void ClearCurse();

	// GrantKillRewards: 마지막으로 이 몬스터를 때린 플레이어(소환수가 때렸으면 그 주인)에게 경험치와 돈을 지급
	void GrantKillRewards();

	// LastRewardPlayer: 처치 보상을 받을 플레이어. 플레이어 본인이나 플레이어 소환수에게 맞을 때마다 갱신
	TWeakObjectPtr<AActor> LastRewardPlayer;

	FTimerHandle CurseTimerHandle;
	FTimerHandle SummonAppearTimerHandle;
	FTimerHandle HPBarHideTimerHandle;

	// ShowHPBarAfterDamage: 피해를 받았을 때 HP바를 보이고, 항상 표시 대상이 아니면 HPBarHideDelay 뒤에 숨김 예약
	void ShowHPBarAfterDamage();

	// SpawnDamageNumber: 머리 위에 피해 숫자를 띄움. 플레이어가 때리면 크고 노란색, 소환수가 때리면 흰색, 내 소환수가 맞으면 빨간색
	void SpawnDamageNumber(float Damage, const AActor* DamageCauser) const;

	UPROPERTY()
	UNiagaraComponent* CurseVFXComponent = nullptr;
};
