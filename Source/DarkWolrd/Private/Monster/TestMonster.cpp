// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/TestMonster.h"
#include "Components/SkeletalMeshComponent.h"

// 생성자: 눈으로 확인 가능하도록 기존 매니퀸 메시를 부착. 스탯은 BaseMonster의 기본값을 그대로 사용
ATestMonster::ATestMonster()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));

	if (MannyMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMeshAsset.Object);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}
