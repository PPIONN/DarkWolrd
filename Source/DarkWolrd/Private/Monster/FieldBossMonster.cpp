// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/FieldBossMonster.h"
#include "AI/WildMonsterAIComponent.h"
#include "UI/GameAnnouncementSubsystem.h"

// 필드보스 기본값: 일반 몬스터보다 훨씬 튼튼하고, 더 멀리서 알아차리고, 더 멀리까지 쫓아옴. 수치는 보스 BP에서 조정
AFieldBossMonster::AFieldBossMonster()
{
	Grade = EMonsterGrade::Rare;
	BaseMaxHP = 1500.f;
	HPPerLevel = 100.f;
	BaseAttackPower = 20.f;
	AttackPowerPerLevel = 5.f;
	RewardEXP = 300.f;
	CardDropChance = 0.3f;

	if (WildAIComponent)
	{
		WildAIComponent->AggroRadius = 1200.f;
		WildAIComponent->LeashRadius = 3000.f;
		WildAIComponent->PatrolRadius = 400.f;
	}
}


FText AFieldBossMonster::GetBossDisplayName() const
{
	if (!BossName.IsEmpty()) return BossName;

	FString ClassName = GetClass()->GetName();
	ClassName.RemoveFromEnd(TEXT("_C"));
	return FText::FromString(ClassName);
}


// 피격: 피해를 받은 뒤 HP 비율을 확인해서 페이즈 전환
float AFieldBossMonster::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		CheckPhaseTransitions();
	}
	return ActualDamage;
}


// 사망: 설정에 따라 불러낸 부하도 함께 정리한 뒤 일반 몬스터 사망 처리(보상, 카드, 사망 애니메이션)
void AFieldBossMonster::Die()
{
	if (bRemoveMinionsOnDeath)
	{
		for (const TWeakObjectPtr<ABaseMonster>& WeakMinion : SpawnedMinions)
		{
			if (ABaseMonster* Minion = WeakMinion.Get())
			{
				if (Minion->IsAlive())
				{
					Minion->Destroy();
				}
			}
		}
	}
	SpawnedMinions.Empty();

	Super::Die();
}


// 페이즈 전환: HPPercent가 높은 페이즈부터 차례로, 현재 HP 비율 이하인 것을 전부 적용 (큰 한 방에 두 단계를 넘기는 경우 포함).
// 플레이어 소환수로 불려 나온 보스는 페이즈를 쓰지 않음
void AFieldBossMonster::CheckPhaseTransitions()
{
	if (bIsPlayerSummon || !IsAlive() || EnteredPhaseCount >= Phases.Num()) return;

	TArray<int32> PhaseOrder;
	for (int32 PhaseIndex = 0; PhaseIndex < Phases.Num(); ++PhaseIndex)
	{
		PhaseOrder.Add(PhaseIndex);
	}
	PhaseOrder.Sort([this](int32 A, int32 B) { return Phases[A].HPPercent > Phases[B].HPPercent; });

	const float HPRatio = CurrentHP / FMath::Max(GetMaxHP(), 1.f);
	while (EnteredPhaseCount < PhaseOrder.Num() && HPRatio <= Phases[PhaseOrder[EnteredPhaseCount]].HPPercent)
	{
		EnterPhase(Phases[PhaseOrder[EnteredPhaseCount]]);
		++EnteredPhaseCount;
	}
}


void AFieldBossMonster::EnterPhase(const FBossPhase& Phase)
{
	AttackPowerBuffMultiplier *= FMath::Max(Phase.AttackPowerMultiplier, 0.f);

	if (WildAIComponent)
	{
		if (Phase.AttackSpeedMultiplier > 0.f)
		{
			WildAIComponent->AttackInterval /= Phase.AttackSpeedMultiplier;
		}
		WildAIComponent->ScaleBaseMoveSpeed(Phase.MoveSpeedMultiplier);
	}

	ActiveSkills.Append(Phase.AddedSkills);

	if (Phase.EnterMontage)
	{
		const float MontageLength = PlayAnimMontage(Phase.EnterMontage);
		if (WildAIComponent && MontageLength > 0.f)
		{
			WildAIComponent->LockActions(MontageLength);
		}
	}

	if (!Phase.EnterAnnouncement.IsEmpty())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Boss"), GetBossDisplayName());
		UGameAnnouncementSubsystem::AnnounceInWorld(this, FText::Format(Phase.EnterAnnouncement, Args), 4.f);
	}

	// 부하 소환: 보스 주변에 원형으로 배치하고 보스와 같은 진영으로 설정 (야생 AI가 알아서 싸움)
	if (Phase.MinionClass && Phase.MinionCount > 0)
	{
		for (int32 MinionIndex = 0; MinionIndex < Phase.MinionCount; ++MinionIndex)
		{
			const float Angle = 2.f * PI * MinionIndex / Phase.MinionCount;
			const FVector Offset(FMath::Cos(Angle) * Phase.MinionSpawnRadius, FMath::Sin(Angle) * Phase.MinionSpawnRadius, 0.f);
			const FTransform SpawnTransform(GetActorRotation(), GetActorLocation() + Offset);

			ABaseMonster* Minion = GetWorld()->SpawnActorDeferred<ABaseMonster>(Phase.MinionClass, SpawnTransform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!Minion) continue;

			Minion->Faction = Faction;
			Minion->FinishSpawning(SpawnTransform);
			SpawnedMinions.Add(Minion);
		}
	}
}
