// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicOrbProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

// AMagicOrbProjectile: 마법사 캐릭터가 시전하는 빛나는 구슬.
// 캐스팅이 끝나면 캐릭터에 부착된 채로 대기하다가, Launch()가 호출되면 지정된 방향으로 날아가 적을 맞춘다.
UCLASS()
class DARKWOLRD_API AMagicOrbProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMagicOrbProjectile();

	// Launch: 대기 중이던 구슬을 지정한 방향으로 발사함. 발사 이전에는 충돌·이동 판정을 하지 않음
	void Launch(const FVector& Direction);

protected:
	virtual void BeginPlay() override;

	// OnCollisionSphereBeginOverlap: 발사된 구슬이 다른 액터와 겹쳤을 때 데미지를 주고 스스로 파괴됨
	UFUNCTION()
	void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// ApplyOnHitEffect: 적중 시 이 스킬만의 고유 효과(화상, 기절 등)를 적용하는 훅.
	// 스킬마다 효과가 완전히 다를 수 있으므로 기본 구현은 데미지 외에 아무것도 하지 않으며,
	// 화상을 주는 스킬은 이걸 재정의한 서브클래스(C++ 또는 블루프린트)를 만들어 사용한다.
	UFUNCTION(BlueprintNativeEvent, Category = "Magic Orb")
	void ApplyOnHitEffect(AActor* HitActor);
	virtual void ApplyOnHitEffect_Implementation(AActor* HitActor) {}

	// GetCasterSkillLevel: 이 오브를 쏜 캐스터(Owner)의 AbilityComponent에서 SourceAbilityID의 현재 스킬 레벨을 조회.
	// 캐스터나 컴포넌트를 못 찾거나 아직 배우지 않은 스킬(0)이면 최소 1로 취급 (온히트 효과가 0으로 나눠지지 않도록)
	int32 GetCasterSkillLevel() const;

public:
	// CollisionSphere: 충돌 판정용 구체 (루트 컴포넌트)
	UPROPERTY(VisibleAnywhere, Category = "Magic Orb")
	USphereComponent* CollisionSphere;

	// OrbMesh: 빛나는 구슬의 외형. 에디터에서 스태틱 메시와 발광(Emissive) 머티리얼을 지정해서 사용
	UPROPERTY(VisibleAnywhere, Category = "Magic Orb")
	UStaticMeshComponent* OrbMesh;

	// ProjectileMovement: 발사 후 직선으로 날아가는 이동을 담당. 발사 전까지는 비활성 상태
	UPROPERTY(VisibleAnywhere, Category = "Magic Orb")
	UProjectileMovementComponent* ProjectileMovement;

	// Damage: 적중 시 줄 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic Orb")
	float Damage = 25.f;

	// Speed: 발사 후 날아가는 속도 (초당 유닛)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic Orb")
	float Speed = 1600.f;

	// LifeSpanAfterLaunch: 발사 후 아무것도 맞추지 못했을 때 자동 소멸까지 걸리는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic Orb")
	float LifeSpanAfterLaunch = 5.f;

	// ImpactVFX: 바닥이든 몬스터든 무엇에 부딫혀 사라지든 공통으로 재생되는 충돌 이펙트 (에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magic Orb")
	UNiagaraSystem* ImpactVFX;

	// SourceAbilityID: 이 오브를 발사한 어빌리티의 ID("Curse" 등). AbilityComponent가 스폰 시 채워줌.
	// 온히트 효과(저주 등)에서 캐스터의 스킬 레벨을 조회할 때 이 ID로 찾음
	UPROPERTY(BlueprintReadOnly, Category = "Magic Orb")
	FName SourceAbilityID = NAME_None;

private:
	// bLaunched: 발사되었는지 여부. false인 동안(캐스팅 완료 후 대기 중)에는 충돌 판정을 무시함
	bool bLaunched = false;
};
