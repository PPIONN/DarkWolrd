// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/MonsterCardData.h"
#include "MonsterCompanionComponent.generated.h"

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

	// AddCard: 필드에서 카드를 주워 보유 목록에 추가 (아직 등록 전 상태)
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

private:
	// SpawnSummons: SelectedSummonSlots(또는 비어있으면 도감 앞에서부터)로 소환수를 스폰하고 활성화함
	void SpawnSummons();

	// DestroyActiveSummons: 소환수 액터만 파괴함 (파트너 패시브 갱신은 호출하는 쪽에서 한 번만)
	void DestroyActiveSummons();
};
