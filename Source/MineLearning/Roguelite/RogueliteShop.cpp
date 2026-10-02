#include "RogueliteShop.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

ARogueliteShop::ARogueliteShop()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionArea = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionArea"));
	SetRootComponent(InteractionArea);
	InteractionArea->SetBoxExtent(FVector(220.f, 220.f, 180.f));
	InteractionArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionArea->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARogueliteShop::Entered);
	InteractionArea->OnComponentEndOverlap.AddUniqueDynamic(this, &ARogueliteShop::Left);
}

ARogueliteShop* ARogueliteShop::FindNearby(const APawn* Pawn)
{
	if (!IsValid(Pawn))
	{
		return nullptr;
	}
	ARogueliteShop* Nearest = nullptr;
	double Distance = TNumericLimits<double>::Max();
	for (TActorIterator<ARogueliteShop> Shop(Pawn->GetWorld()); Shop; ++Shop)
	{
		// Use the same overlap fact as the range notifications, including the pawn's capsule.
		if (!Shop->InteractionArea->IsOverlappingActor(Pawn))
		{
			continue;
		}
		const double Candidate = FVector::DistSquared(Pawn->GetActorLocation(), Shop->GetActorLocation());
		if (Candidate < Distance)
		{
			Nearest = *Shop;
			Distance = Candidate;
		}
	}
	return Nearest;
}

void ARogueliteShop::Entered(UPrimitiveComponent* Component, AActor* Actor, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bSweep, const FHitResult& Hit)
{
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		OnRangeChanged.Broadcast(Pawn, true);
	}
}

void ARogueliteShop::Left(UPrimitiveComponent* Component, AActor* Actor, UPrimitiveComponent* OtherComponent, int32 BodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		OnRangeChanged.Broadcast(Pawn, false);
	}
}
