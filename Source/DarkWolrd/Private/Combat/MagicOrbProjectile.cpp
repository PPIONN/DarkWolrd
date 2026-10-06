// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/MagicOrbProjectile.h"
#include "Combat/AbilityComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

// 생성자: 충돌 구체를 루트로 하고, 외형 메시와 발사 이동 컴포넌트를 붙임
AMagicOrbProjectile::AMagicOrbProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(20.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	SetRootComponent(CollisionSphere);

	// OrbMesh: 기본값은 비어있으며, 에디터/블루프린트에서 빛나는 구체 메시와 머티리얼을 지정해서 사용
	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(CollisionSphere);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bAutoActivate = false;
}

// 게임 시작 시: 겹침 이벤트를 바인딩. 발사 전까지는 bLaunched가 false라 데미지 판정을 하지 않음
void AMagicOrbProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMagicOrbProjectile::OnCollisionSphereBeginOverlap);
}

// Launch: 캐스터에게서 분리하고 지정한 방향으로 발사 속도를 부여함
void AMagicOrbProjectile::Launch(const FVector& Direction)
{
	if (bLaunched) return;
	bLaunched = true;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * Speed;

	SetLifeSpan(LifeSpanAfterLaunch);
}

// 겹침 처리: 발사되지 않았거나 자기 자신·소유자와의 겹침은 무시. 그 외에는 데미지를 주고 구슬을 파괴
void AMagicOrbProjectile::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bLaunched) return;
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner()) return;

	UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, nullptr);
	ApplyOnHitEffect(OtherActor);

	// 충돌 지점(스윕이었다면 실제 충돌 좌표, 아니면 오브 위치)에 공통 충돌 이펙트 재생 후 파괴.
	// 바닥이든 몬스터든 무엇에 맞아 사라지든 동일하게 적용됨
	if (ImpactVFX)
	{
		const FVector ImpactLocation = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactVFX, ImpactLocation, GetActorRotation());
	}

	Destroy();
}


// 캐스터의 AbilityComponent에서 이 오브를 쏜 어빌리티의 현재 스킬 레벨을 조회.
// 캐스터/컴포넌트를 못 찾거나 아직 배우지 않았으면(0) 최소 1로 취급
int32 AMagicOrbProjectile::GetCasterSkillLevel() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (UAbilityComponent* Ability = OwnerActor->FindComponentByClass<UAbilityComponent>())
		{
			return FMath::Max(Ability->GetSkillLevel(SourceAbilityID), 1);
		}
	}

	return 1;
}
