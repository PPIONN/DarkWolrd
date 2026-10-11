// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DamageNumberActor.h"
#include "UI/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

ADamageNumberActor::ADamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	SetRootComponent(WidgetComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetWidgetClass(UDamageNumberWidget::StaticClass());
}


void ADamageNumberActor::SpawnDamageNumber(UWorld* World, const FVector& Location, float Damage, const FLinearColor& Color, int32 FontSize)
{
	if (!World) return;

	// 같은 자리에 연속으로 맞아도 숫자가 겹치지 않도록 시작 위치와 흘러가는 방향을 조금씩 흩뜨림
	const FVector Jitter(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(0.f, 35.f));

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADamageNumberActor* Number = World->SpawnActor<ADamageNumberActor>(ADamageNumberActor::StaticClass(), Location + Jitter, FRotator::ZeroRotator, SpawnParams);
	if (!Number) return;

	Number->StartLocation = Number->GetActorLocation();
	Number->SideDrift = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal() * 25.f;

	if (UDamageNumberWidget* Widget = Cast<UDamageNumberWidget>(Number->WidgetComponent->GetUserWidgetObject()))
	{
		Widget->SetDamage(Damage, Color, FontSize);
		Widget->SetRenderScale(FVector2D(Number->PopScale));
	}
}


// 매 프레임: 처음엔 크게 튀어나왔다가 원래 크기로, 위로 떠오르며(점점 느려짐) 후반부에 투명해짐
void ADamageNumberActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;
	const float Alpha = FMath::Clamp(Age / LifeTime, 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		Destroy();
		return;
	}

	const float RiseAlpha = 1.f - FMath::Square(1.f - Alpha);
	SetActorLocation(StartLocation + FVector(0.f, 0.f, RiseDistance * RiseAlpha) + SideDrift * RiseAlpha);

	if (UUserWidget* Widget = WidgetComponent->GetUserWidgetObject())
	{
		const float PopAlpha = PopTime > 0.f ? FMath::Clamp(Age / PopTime, 0.f, 1.f) : 1.f;
		Widget->SetRenderScale(FVector2D(FMath::Lerp(PopScale, 1.f, PopAlpha)));
		Widget->SetRenderOpacity(Alpha < 0.6f ? 1.f : 1.f - (Alpha - 0.6f) / 0.4f);
	}
}
