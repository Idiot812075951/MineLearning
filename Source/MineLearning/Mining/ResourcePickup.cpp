#include "ResourcePickup.h"

#include "ResourceCarryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AResourcePickup::AResourcePickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

void AResourcePickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAttachMovement(DeltaSeconds);
}

void AResourcePickup::InitializeResource(EResourceType InType, int32 InAmount)
{
	ResourceType = InType;
	Amount = InAmount;
}

bool AResourcePickup::TryReserve(AActor* Collector)
{
	if (!IsAvailableFor(Collector))
	{
		return false;
	}

	ReservedCollector = Collector;
	return true;
}

bool AResourcePickup::IsAvailableFor(AActor* Collector) const
{
	if (!IsValid(Collector) || Amount <= 0)
	{
		return false;
	}

	return !ReservedCollector.IsValid() || ReservedCollector.Get() == Collector;
}

void AResourcePickup::ReleaseReservation(AActor* Collector)
{
	if (ReservedCollector.Get() == Collector)
	{
		ReservedCollector.Reset();
	}
}

bool AResourcePickup::AttachToCollector(USkeletalMeshComponent* CollectorMesh, FName SocketName)
{
	AActor* Collector = CollectorMesh ? CollectorMesh->GetOwner() : nullptr;
	if (!CollectorMesh
		|| !CollectorMesh->DoesSocketExist(SocketName)
		|| !IsAvailableFor(Collector)
		|| ReservedCollector.Get() != Collector)
	{
		return false;
	}

	PickupSphere->SetGenerateOverlapEvents(false);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (AttachToComponent(CollectorMesh, FAttachmentTransformRules::KeepWorldTransform, SocketName))
	{
		SetActorTickEnabled(true);
		return true;
	}

	CancelCollect(Collector);
	return false;
}

void AResourcePickup::CancelCollect(AActor* Collector)
{
	if (ReservedCollector.IsValid() && ReservedCollector.Get() != Collector)
	{
		return;
	}

	SetActorTickEnabled(false);

	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetEnableGravity(true);
	Mesh->SetSimulatePhysics(true);

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetGenerateOverlapEvents(true);

	ReleaseReservation(Collector);
}

void AResourcePickup::UpdateAttachMovement(float DeltaSeconds)
{
	USceneComponent* Root = GetRootComponent();
	USceneComponent* AttachParent = Root ? Root->GetAttachParent() : nullptr;
	if (!Root || !AttachParent)
	{
		SetActorTickEnabled(false);
		return;
	}

	const FTransform TargetTransform = AttachParent->GetSocketTransform(
		Root->GetAttachSocketName(),
		RTS_World
	);

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const float Distance = FVector::Distance(CurrentLocation, TargetLocation);
	const float MoveStep = FMath::Max(AttachMoveSpeed, 1.0f) * DeltaSeconds;
	const float Alpha = Distance > KINDA_SMALL_NUMBER
		? FMath::Clamp(MoveStep / Distance, 0.0f, 1.0f)
		: 1.0f;

	const FVector NewLocation = FMath::Lerp(CurrentLocation, TargetLocation, Alpha);
	const FQuat NewRotation = FQuat::Slerp(GetActorQuat(), TargetTransform.GetRotation(), Alpha);
	SetActorLocationAndRotation(NewLocation, NewRotation);

	if (Alpha >= 1.0f)
	{
		Root->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		SetActorTickEnabled(false);
	}
}

bool AResourcePickup::TryCollect(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == this || !IsAvailableFor(OtherActor))
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
