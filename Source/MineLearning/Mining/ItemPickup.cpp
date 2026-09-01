#include "ItemPickup.h"

#include "ItemReceiver.h"
#include "MineLearning/AI/HaulerCharacter.h"
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

void AItemPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ReservationSourceStorage) && ItemStack.IsValid())
	{
		ReservationSourceStorage->ReleaseReservedItem(ItemStack);
	}
	ReservationSourceStorage = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAttachMovement(DeltaSeconds);
}

void AItemPickup::InitializeItem(
	const FItemStack& InItemStack,
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes)
{
	SetItemStack(InItemStack);
	SetActorScale3D(FVector::OneVector);
	SelectDropMesh(InDropMeshes);
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
	if (!IsValid(ExplicitDeliveryActor) || !IsValid(ExplicitDeliveryPoint))
	{
		return false;
	}

	if (IsValid(ExplicitDeliveryStorage))
	{
		return ExplicitDeliveryStorage->GetOwner() == ExplicitDeliveryActor
			&& ExplicitDeliveryStorage->CanAddItem(ItemStack);
	}

	return ExplicitDeliveryActor->GetClass()->ImplementsInterface(UItemReceiver::StaticClass())
		&& IItemReceiver::Execute_CanAcceptItem(ExplicitDeliveryActor, ItemStack);
}

void AItemPickup::SetReservationSource(UResourceStorageComponent* InSourceStorage)
{
	ReservationSourceStorage = InSourceStorage;
}

int32 AItemPickup::CancelReservedAmount(int32 Amount)
{
	if (!IsValid(ReservationSourceStorage) || Amount <= 0 || !ItemStack.IsValid())
	{
		return 0;
	}

	FItemStack CanceledItem = ItemStack;
	CanceledItem.Amount = FMath::Min(Amount, ItemStack.Amount);
	if (!ReservationSourceStorage->ReleaseReservedItem(CanceledItem))
	{
		return 0;
	}

	ItemStack.Amount -= CanceledItem.Amount;
	if (ItemStack.Amount <= 0)
	{
		ReservationSourceStorage = nullptr;
		Destroy();
	}
	return CanceledItem.Amount;
}

void AItemPickup::SetWaitingVisualEnabled(bool bEnabled)
{
	Mesh->SetVisibility(bEnabled, true);
	Mesh->SetHiddenInGame(!bEnabled, true);
	PickupSphere->SetCollisionEnabled(
		bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	PickupSphere->SetGenerateOverlapEvents(bEnabled);
}

void AItemPickup::SelectDropMesh(
	const TArray<TObjectPtr<UStaticMesh>>& InDropMeshes)
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
	Mesh->SetRelativeScale3D(FVector(
		MineLearningItemVisual::GetUniformScale(SelectedDropMesh)));
}

bool AItemPickup::TryReserve(AActor* Collector)
{
	if (!IsAvailableFor(Collector))
	{
		return false;
	}

	ReservedCollector = Collector;
	return true;
}

bool AItemPickup::IsAvailableFor(AActor* Collector) const
{
	if (bTransportLocked || !IsValid(Collector) || !ItemStack.IsValid())
	{
		return false;
	}
	if (bRequiresHauler && !Collector->IsA<AHaulerCharacter>())
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

	const int32 ExpectedAmount = FMath::Min(
		ItemStack.Amount,
		CarryComponent->GetCapacity() - CarryComponent->GetCurrentItemCount());
	FItemStack ReservedTransfer = ItemStack;
	ReservedTransfer.Amount = ExpectedAmount;
	if (IsValid(ReservationSourceStorage)
		&& !ReservationSourceStorage->CanCommitReservedItem(ReservedTransfer))
	{
		return false;
	}

	const int32 AddedAmount = CarryComponent->AddItemWithVisual(
		ItemStack,
		SelectedDropMesh);
	if (AddedAmount <= 0)
	{
		return false;
	}
	if (IsValid(ReservationSourceStorage))
	{
		FItemStack CommittedItem = ItemStack;
		CommittedItem.Amount = AddedAmount;
		if (!ensure(ReservationSourceStorage->CommitReservedItem(CommittedItem)))
		{
			return false;
		}
	}

	ItemStack.Amount -= AddedAmount;
	if (ItemStack.Amount <= 0)
	{
		ReservationSourceStorage = nullptr;
		Destroy();
	}

	return true;
}
