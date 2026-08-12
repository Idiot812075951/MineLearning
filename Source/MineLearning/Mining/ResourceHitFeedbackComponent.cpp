#include "ResourceHitFeedbackComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UResourceHitFeedbackComponent::UResourceHitFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitFinder(TEXT("/Game/FX/Gunner/NS_ResourceStoneHit.NS_ResourceStoneHit"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DestroyedFinder(TEXT("/Game/FX/Gunner/NS_GunnerGoldenHeadshot.NS_GunnerGoldenHeadshot"));
	HitSystem = HitFinder.Object;
	DestroyedSystem = DestroyedFinder.Object;
}

void UResourceHitFeedbackComponent::PlayHitFeedback(FVector Location, FVector Normal, float Strength)
{
	if (HitSystem && GetWorld())
	{
		const float Scale = FMath::Clamp(FMath::Sqrt(FMath::Max(Strength, 1.0f)) * 0.12f, 0.45f, 1.25f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitSystem, Location, Normal.Rotation(), FVector(Scale), true, true);
	}
}

void UResourceHitFeedbackComponent::PlayDestroyedFeedback(FVector Location, FVector Normal)
{
	if (DestroyedSystem && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DestroyedSystem, Location, Normal.Rotation(), FVector::OneVector, true, true);
	}
}
