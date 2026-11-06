// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/MonsterCardData.h"
#include "MonsterCompanionComponent.generated.h"

class UNiagaraSystem;
class USoundBase;
class UAnimMontage;

// UMonsterCompanionComponent: 플레이어가 주운/등록한 몬스터 카드를 관리하는 컴포넌트.
// HeldCards = 주웠지만 아직 등록(소환 해금)하지 않은 카드, RegisteredMonsters = 영구 등록된 도감.
// 소환/포메이션/전투 AI는 다음 단계에서 이 도감 데이터를 기반으로 구현할 예정
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UMonsterCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterCompanionComponent();

	// MaxUpgradeCount: 같은 몬스터를 추가로 몇 번까지 등록해 업그레이드할 수 있는지 (4~5회 상당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	int32 MaxUpgradeCount = 4;

	// HeldCards: 주웠지만 아직 등록하지 않은 카드 목록
	UPROPERTY(BlueprintReadOnly, Category = "Monster Card")
	TArray<FMonsterCardEntry> HeldCards;

	// RegisteredMonsters: 카드를 소모해 영구로 등록한 몬스터 도감. 같은 종족은 UpgradeCount만 증가
	UPROPERTY(BlueprintReadOnly, Category = "Monster Card")
	TArray<FRegisteredMonsterEntry> RegisteredMonsters;

	// bAutoRegisterCards: 카드를 주우면 바로 도감에 등록(신규 종족 해금 또는 강화). 끄면 보유 목록에만 넣고 등록은 따로 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	bool bAutoRegisterCards = true;

	// WildKillCount: 플레이어와 소환수가 쓰러뜨린 야생 몬스터 수 (튜토리얼 목표 판정용)
	UPROPERTY(BlueprintReadOnly, Category = "Monster Card")
	int32 WildKillCount = 0;

	// CardPickupVFX / CardPickupSound: 카드를 주운 순간 플레이어 발밑에서 재생. 비워두면 생략
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	UNiagaraSystem* CardPickupVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster Card")
	USoundBase* CardPickupSound = nullptr;

	// AddCard: 필드에서 카드를 주워 보유 목록에 추가 (bAutoRegisterCards면 바로 등록까지)
	UFUNCTION(BlueprintCallable, Category = "Monster Card")
	void AddCard(const FMonsterCardEntry& CardData);

	// RegisterCard: 보유 카드 하나를 소모해서 신규 등록하거나(처음 보는 종족) 업그레이드(이미 등록된 종족)함.
	// 성공하면 true. HeldCardIndex가 유효하지 않으면 false
	UFUNCTION(BlueprintCallable, Category = "Monster Card")
	bool RegisterCard(int32 HeldCardIndex);

	// RegisterAllHeldCards: 보유 카드를 전부 등록함 (카드 목록 UI가 생기기 전 임시 등록 수단 겸 테스트용). 등록한 장 수를 반환
	UFUNCTION(BlueprintCallable, Category = "Monster Card")
	int32 RegisterAllHeldCards();

	// FindRegisteredEntry: 등록된 도감에서 해당 종족 항목을 찾음 (없으면 nullptr)
	FRegisteredMonsterEntry* FindRegisteredEntry(TSubclassOf<ABaseMonster> MonsterClass);

	// ─── 소환 시스템 ───────────────────────────────────────

	// MaxActiveSummons: 동시에 소환할 수 있는 최대 마리 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon")
	int32 MaxActiveSummons = 3;

	// SelectedSummonSlots: 등록된 도감(RegisteredMonsters) 중 실제로 소환할 (최대 3개) 종족을 고른 목록.
	// 비어있으면 도감의 앞에서부터 MaxActiveSummons개를 자동으로 사용함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon")
	TArray<TSubclassOf<ABaseMonster>> SelectedSummonSlots;

	// ActiveSummons: 지금 필드에 소환되어 있는 몬스터 액터들
	UPROPERTY(BlueprintReadOnly, Category = "Summon")
	TArray<ABaseMonster*> ActiveSummons;

	// AreSummonsActive: 현재 소환된 몬스터가 하나라도 있는지
	UFUNCTION(BlueprintCallable, Category = "Summon")
	bool AreSummonsActive() const { return ActiveSummons.Num() > 0; }

	// ResummonCooldown: 재소환한 뒤 다시 재소환할 수 있을 때까지의 시간(초). 0이면 제한 없음.
	// Y 연타로 소환수 HP를 계속 채우는 걸 막고 싶을 때 올리면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon")
	float ResummonCooldown = 0.f;

	// ─── 소환 연출 ───────────────────────────────────────

	// SummonVFX / SummonSound: 소환수가 나타날 때 발밑에서 재생하는 공통 이펙트·사운드 (몬스터 BP에 따로 지정돼 있으면 그쪽 우선)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	UNiagaraSystem* SummonVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	USoundBase* SummonSound = nullptr;

	// DespawnVFX / DespawnSound: 재소환·소환 해제로 소환수가 사라질 때 그 자리에서 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	UNiagaraSystem* DespawnVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	USoundBase* DespawnSound = nullptr;

	// SummonCastMontage: 소환할 때 플레이어(토끼)가 재생할 몽타주 (손을 뻗는 시전 동작 등). 비워두면 재생 안 함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	UAnimMontage* SummonCastMontage = nullptr;

	// SummonAppearDuration: 소환수 몸이 작은 크기에서 튀어나오듯 커지는 시간(초). 0이면 크기 연출 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	float SummonAppearDuration = 0.35f;

	// SummonStagger: 여러 마리를 소환할 때 한 마리씩 차례로 나타나는 간격(초). 0이면 동시에 나타남
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|FX")
	float SummonStagger = 0.2f;

	// SetSummonLocked: 켜두면 Y 재소환이 막힘 (몬스터대전 중 재소환으로 HP를 채우거나 판을 뒤엎는 것 방지)
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void SetSummonLocked(bool bLocked) { bSummonLocked = bLocked; }

	UFUNCTION(BlueprintCallable, Category = "Summon")
	bool IsSummonLocked() const { return bSummonLocked; }

	// GetResummonCooldownRemaining: 다시 재소환할 수 있을 때까지 남은 시간(초). 0이면 지금 가능 (UI 표시용)
	UFUNCTION(BlueprintCallable, Category = "Summon")
	float GetResummonCooldownRemaining() const;

	// Resummon: Y 입력 처리. 지금 소환된 몬스터를 전부 취소하고, 도감에서 최대 MaxActiveSummons마리를
	// 플레이어 주변에 HP가 가득 찬 새 개체로 바로 다시 소환함 (아무것도 소환 안 된 상태면 그냥 소환)
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void Resummon();

	// DespawnAllSummons: 소환된 몬스터를 전부 제거하고 파트너 패시브도 해제.
	// 액터가 파괴되며 그 안에 있던 모든 우선순위 정보도 함께 사라짐
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void DespawnAllSummons();

	// HandleSummonDied: 소환수가 전투 중 쓰러졌을 때 BaseMonster::Die에서 호출. 목록에서 빼고 파트너 패시브를 다시 계산함
	void HandleSummonDied(ABaseMonster* Summon);

	// RefreshPartnerPassives: 지금 살아있는 소환수들의 파트너 패시브를 플레이어 스탯에 다시 반영함
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void RefreshPartnerPassives();

	// AddSummonEXP: 지정한 종족의 등록 도감 항목에 경험치를 더하고, 필요하면 레벨업까지 처리함.
	// 그 종족이 지금 소환되어 필드에 있으면 살아있는 개체의 레벨/최대체력도 즉시 갱신함
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void AddSummonEXP(TSubclassOf<ABaseMonster> MonsterClass, float EXP);

	// HandleGatherCommand: "4+1" 커맨드. 모든 소환수의 우선순위를 초기화하고 플레이어 주변에 모이게 함
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void HandleGatherCommand();

	// HandleAttackPlayerTargetCommand: "4+2" 커맨드. 플레이어가 공격중인 대상만 남기고
	// 모든 소환수의 다른 우선순위(자기방어 등)를 초기화함
	UFUNCTION(BlueprintCallable, Category = "Summon")
	void HandleAttackPlayerTargetCommand();

	// ─── 자동 사냥 ───────────────────────────────────────

	// bSummonAutoHunt: 켜면 소환수가 플레이어 주변(AutoHuntRadius)의 적을 스스로 찾아 계속 사냥함.
	// 플레이어가 가만히 있어도(잠수) 사냥을 이어가며, 적이 없을 때만 따라다니기/배회로 돌아감
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Auto Hunt")
	bool bSummonAutoHunt = true;

	// AutoHuntRadius: 자동 사냥으로 노리는 적의 범위 (플레이어 기준 거리)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Auto Hunt")
	float AutoHuntRadius = 1500.f;

	// SetSummonAutoHunt: 자동 사냥을 켜고 끔. 지금 소환되어 있는 소환수에도 바로 적용 (UI 토글 버튼 등에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Summon|Auto Hunt")
	void SetSummonAutoHunt(bool bEnabled);

	// ApplyAutoHuntSettings: 소환수 AI에 자동 사냥 설정을 적용 (소환할 때, 몬스터대전이 끝났을 때)
	void ApplyAutoHuntSettings(ABaseMonster* Summon) const;

	// ─── 협공 ───────────────────────────────────────

	// AssistMemoryTime: 플레이어가 때린 적 / 플레이어를 때린 적을 이 시간(초) 동안 기억해서 소환수가 함께 공격함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Assist")
	float AssistMemoryTime = 8.f;

	// AssistRadius: 플레이어에게서 이 거리 안에 있는 적만 협공 (멀리 달아난 적을 끝까지 쫓아가지 않게)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Summon|Assist")
	float AssistRadius = 2000.f;

	// NotifyPlayerHitEnemy: 플레이어 본인이 적을 공격했을 때 (BaseMonster::TakeDamage에서 호출)
	void NotifyPlayerHitEnemy(AActor* Enemy);

	// NotifyPlayerAttacked: 적이 플레이어를 공격했을 때 (Rabbit::TakeDamage에서 호출)
	void NotifyPlayerAttacked(AActor* Attacker);

	// GetAssistTarget: 소환수가 함께 공격할 대상 (가장 최근에 플레이어가 때렸거나 플레이어를 때린 적).
	// 기억 시간이 지났거나, 쓰러졌거나, 플레이어에게서 너무 멀면 nullptr
	AActor* GetAssistTarget() const;

private:
	TWeakObjectPtr<AActor> AssistTarget;
	float AssistTargetTime = -1000.f;

	// SpawnSummons: SelectedSummonSlots(또는 비어있으면 도감 앞에서부터)로 소환수를 스폰하고 활성화함
	void SpawnSummons();

	// DestroyActiveSummons: 소환수 액터만 파괴함 (파트너 패시브 갱신은 호출하는 쪽에서 한 번만)
	void DestroyActiveSummons();

	// RevealSummon: 소환수를 보이게 하고 등장 연출(이펙트·사운드·튀어나오기)을 재생
	void RevealSummon(ABaseMonster* Summon);

	// LastResummonTime: 마지막으로 실제 소환에 성공한 시각 (ResummonCooldown 계산용)
	float LastResummonTime = -1000.f;

	bool bSummonLocked = false;
};
