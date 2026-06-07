// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/HP_Potion.h"

// 생성자: Tick을 활성화 (매 프레임 업데이트가 필요할 경우 사용)
AHP_Potion::AHP_Potion()
{
	// Tick을 매 프레임 호출하도록 설정. 필요 없으면 false로 바꾸면 성능이 좋아짐
	PrimaryActorTick.bCanEverTick = true;

}

// 게임 시작 또는 이 액터가 세상에 등장할 때 한 번 실행
void AHP_Potion::BeginPlay()
{
	Super::BeginPlay();

}

// 매 프레임 실행: 현재는 별도 로직 없음
void AHP_Potion::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
