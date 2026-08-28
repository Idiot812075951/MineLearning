#include "ItemPickup.h"

#include "ResourceCarryComponent.h"
#include "ResourceStorageComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AItemPickup::AItemPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCanEverAffectNavigation(false);
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

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// Loose resources remain physical against the ground, but must not seal a
	// narrow logistics aisle or physically block a Pawn.
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void AItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAttachMovement(DeltaSeconds);
}

void AItemPickup::InitializeItem(
	const FItemStack& InItemStack,
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes,
	float InDropMeshScale)
{
	SetItemStack(InItemStack);
	SelectDropMesh(InDropMeshes, InDropMeshScale);
}

void AItemPickup::SetItemStack(const FItemStack& InItemStack)
{
	ItemStack = InItemStack;
	ItemStack.Amount = FMath::Max(ItemStack.Amount, 0);
}

void AItemPickup::SetExplicitDeliveryTarget(
	AActor* InDeliveryActor,
	UResourceStorageComponent* InDeliveryStorage,
	USceneComponent* InDeliveryPoint)
{
	ExplicitDeliveryActor = InDeliveryActor;
	ExplicitDeliveryStorage = InDeliveryStorage;
	ExplicitDeliveryPoint = InDeliveryPoint;
}

bool AItemPickup::HasUsableExplicitDeliveryTarget() const
{
	return IsValid(ExplicitDeliveryActor)
		&& IsValid(ExplicitDeliveryStorage)
		&& IsValid(ExplicitDeliveryPoint)
		&& ExplicitDeliveryStorage->GetOwner() == ExplicitDeliveryActor
		&& ExplicitDeliveryStorage->CanAddItem(ItemStack);
}

void AItemPickup::SelectDropMesh(
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes,
	float InDropMeshScale)
{
	TArray<UStaticMesh*> ValidMeshes;
	ValidMeshes.Reserve(InDropMeshes.Num());
	for (UStaticMesh* DropMesh : InDropMeshes)
	{
		if (IsValid(DropMesh))
		{
			ValidMeshes.Add(DropMesh);
		}
	}

	if (ValidMeshes.IsEmpty())
	{
		SelectedDropMesh = nullptr;
		return;
	}

	SelectedDropMesh = ValidMeshes[FMath::RandRange(0, ValidMeshes.Num() - 1)];
	Mesh->SetStaticMesh(SelectedDropMesh);
	Mesh->SetRelativeScale3D(FVector(FMath::Max(InDropMeshScale, 0.01f)));
}

bool AItemPickup::TryReserve(AActor* Collector)
{
	if (!IsAvailableFor(Collector))
	{
		return false;
	}

	ReservedCollector = Collector;
	ReservedResourceMeshScale = Mesh->GetComponentTransform().GetScale3D().GetAbsMax();
	return true;
}

bool AItemPickup::IsAvailableFor(AActor* Collector) const
{
	if (bTransportLocked || !IsValid(Collector) || !ItemStack.IsValid())
	{
		return false;
	}

	return !ReservedCollector.IsValid() || ReservedCollector.Get() == Collector;
}

void AItemPickup::SetTransportLocked(bool bLocked)
{
	bTransportLocked = bLocked;
	ReservedCollector.Reset();
	SetActorTickEnabled(false);

	if (bTransportLocked)
	{
		PickupSphere->SetGenerateOverlapEvents(false);
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Mesh->SetSimulatePhysics(false);
		Mesh->SetEnableGravity(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetEnableGravity(true);
	Mesh->SetSimulatePhysics(true);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void AItemPickup::ReleaseStationaryForCollection()
{
	bTransportLocked = false;
	ReservedCollector.Reset();
	SetActorTickEnabled(false);

	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCanEverAffectNavigation(false);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void AItemPickup::ReleaseReservation(AActor* Collector)
{
	if (ReservedCollector.Get() == Collector)
	{
		ReservedCollector.Reset();
		ReservedResourceMeshScale = 1.0f;
	}
}

bool AItemPickup::AttachToCollector(USkeletalMeshComponent* CollectorMesh, FName SocketName)
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

void AItemPickup::CancelCollect(AActor* Collector)
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
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetEnableGravity(true);
	Mesh->SetSimulatePhysics(true);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetGenerateOverlapEvents(true);
	ReleaseReservation(Collector);
}

void AItemPickup::UpdateAttachMovement(float DeltaSeconds)
{
	USceneComponent* Root = GetRootComponent();
	USceneComponent* AttachParent = Root ? Root->GetAttachParent() : nullptr;
	if (!Root || !AttachParent)
	{
		SetActorTickEnabled(false);
		return;
	}

	const FTransform TargetTransform = AttachParent->GetSocketTransform(
		Root->GetAttachSocketName(), RTS_World);
	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const float Distance = FVector::Distance(CurrentLocation, TargetLocation);
	const float MoveStep = FMath::Max(AttachMoveSpeed, 1.0f) * DeltaSeconds;
	const float Alpha = Distance > KINDA_SMALL_NUMBER
		? FMath::Clamp(MoveStep / Distance, 0.0f, 1.0f)
		: 1.0f;

	SetActorLocationAndRotation(
		FMath::Lerp(CurrentLocation, TargetLocation, Alpha),
		FQuat::Slerp(GetActorQuat(), TargetTransform.GetRotation(), Alpha));

	if (Alpha >= 1.0f)
	{
		Root->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		SetActorTickEnabled(false);
	}
}

bool AItemPickup::TryCollect(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == this || !IsAvailableFor(OtherActor))
	{
		return false;
	}

	if (!ItemStack.IsValid())
	{
		Destroy();
		return false;
	}

	UResourceCarryComponent* CarryComponent = OtherActor->FindComponentByClass<UResourceCarryComponent>();
	if (!CarryComponent)
	{
		return false;
	}

	if (!CarryComponent->CanAcceptItem(ItemStack))
	{
		return false;
	}

	const float ResourceMeshScale = ReservedCollector.Get() == OtherActor
		? ReservedResourceMeshScale
		: Mesh->GetComponentTransform().GetScale3D().GetAbsMax();
	const int32 AddedAmount = CarryComponent->AddItemWithVisual(
		ItemStack,
		SelectedDropMesh,
		ResourceMeshScale);
	if (AddedAmount <= 0)
	{
		return false;
	}

	ItemStack.Amount -= AddedAmount;
	if (ItemStack.Amount <= 0)
	{
		Destroy();
	}

	return true;
}
