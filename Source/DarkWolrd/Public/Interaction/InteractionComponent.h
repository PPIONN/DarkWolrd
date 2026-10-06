// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class ANPCBase;

// UInteractionComponent: NPC와의 상호작용(대화)을 담당하는 컴포넌트.
// 매 프레임 "조준선이 NPC를 향하고 있는지" + "그 NPC가 상호작용 범위 안에 있는지"를 둘 다 확인해서,
// 둘 다 만족할 때만 CurrentInteractableNPC를 채워준다 (HUD의 "F: 대화하기" 프롬프트 표시에 사용)
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DARKWOLRD_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// InteractionRange: NPC와의 실제 거리가 이 이내여야 상호작용 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 250.f;

	// InteractionTraceDistance: 조준 트레이스의 최대 거리 (이보다 멀리 있는 NPC는 조준 자체가 안 됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionTraceDistance = 500.f;

	// CurrentInteractableNPC: 지금 이 순간 상호작용 가능한 NPC (범위+조준 둘 다 만족). 없으면 nullptr.
	// HUD에서 이 값을 읽어 "F: 대화하기" 프롬프트를 표시하면 됨
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	ANPCBase* CurrentInteractableNPC;

	// TryInteract: F 입력 처리. 상호작용 가능한 NPC가 있으면 그 NPC의 다음 대사를 진행시키고 true 반환
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract(FText& OutNPCName, FText& OutLine, bool& bOutIsLastLine);

private:
	// UpdateInteractableTarget: 조준 트레이스 + 거리 확인으로 CurrentInteractableNPC를 매 프레임 갱신
	void UpdateInteractableTarget();
};
