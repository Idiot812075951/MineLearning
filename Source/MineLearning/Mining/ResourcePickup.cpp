#include "ResourcePickup.h"

#include "ResourceCarryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AResourcePickup::AResourcePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(Mesh);
	PickupSphere->SetSphereRadius(80.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&AResourcePickup::OnPickupSphereBeginOverlap
	);
}

void AResourcePickup::InitializeResource(EResourceType InType, int32 InAmount)
{
	ResourceType = InType;
	Amount = InAmount;
}

void AResourcePickup::OnPickupSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	TryCollect(OtherActor);
}

bool AResourcePickup::TryCollect(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == this)
	{
		return false;
	}

	if (Amount <= 0)
	{
		Destroy();
		return false;
	}

	UResourceCarryComponent* CarryComponent = OtherActor->FindComponentByClass<UResourceCarryComponent>();
	if (!CarryComponent)
	{
		return false;
	}

	const int32 AddedAmount = CarryComponent->AddOre(Amount);
	if (AddedAmount <= 0)
	{
		return false;
	}

	Amount -= AddedAmount;

	if (Amount <= 0)
	{
		Destroy();
	}

	return true;
}
