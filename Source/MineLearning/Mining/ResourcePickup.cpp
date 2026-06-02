#include "ResourcePickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

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
}

void AResourcePickup::InitializeResource(EResourceType InType, int32 InAmount)
{
	ResourceType = InType;
	Amount = InAmount;
}

void AResourcePickup::BeginPlay()
{
	Super::BeginPlay();
	
	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AResourcePickup::OnPickupSphereBeginOverlap
		);
	}
}

void AResourcePickup::OnPickupSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// 第一版：只要 Pawn 碰到就拾取
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Picked resource. Type=%d Amount=%d"),
		static_cast<int32>(ResourceType),
		Amount
	);

	Destroy();
}
