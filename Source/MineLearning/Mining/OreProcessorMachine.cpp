#include "OreProcessorMachine.h"

#include "ItemPickup.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AOreProcessorMachine::AOreProcessorMachine()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	InputOrePickupClass = AItemPickup::StaticClass();
	OutputPickupClass = AItemPickup::StaticClass();
	ProcessorDisplayName = NSLOCTEXT("MineLearning", "OreProcessorDisplayName", "加工机");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> IronOreMeshFinder(
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01"));
	if (IronOreMeshFinder.Succeeded())
	{
		InputOreMesh = IronOreMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> IronIngotMeshFinder(
		TEXT("/Game/MineLearning/Mining/Resources/IronIngot/SM_IronIngot.SM_IronIngot"));
	if (IronIngotMeshFinder.Succeeded())
	{
		OutputIngotMesh = IronIngotMeshFinder.Object;
	}
}

void AOreProcessorMachine::BeginPlay()
{
	Super::BeginPlay();

	InputSpline = FindAuthoredSpline(InputSplineComponentName);
	OutputSpline = FindAuthoredSpline(OutputSplineComponentName);
	CachedInputPoint = FindAuthoredSceneComponent(InputPointComponentName);
	CachedOutputPoint = FindAuthoredSceneComponent(OutputPointComponentName);
	if (!InputSpline || !OutputSpline || !CachedInputPoint || !CachedOutputPoint)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[OreProcessor] Missing authored transport component. Processor=%s InputPoint=%s InputSpline=%s OutputSpline=%s OutputPoint=%s"),
			*GetNameSafe(this), *GetNameSafe(CachedInputPoint), *GetNameSafe(InputSpline),
			*GetNameSafe(OutputSpline), *GetNameSafe(CachedOutputPoint));
	}

	RefreshMachineState();
	RefreshTickEnabled();
}

void AOreProcessorMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FOreProcessorTransportItem& Item : InputTransportItems)
	{
		if (IsValid(Item.Pickup))
		{
			Item.Pickup->Destroy();
		}
	}
	for (FOreProcessorTransportItem& Item : OutputTransportItems)
	{
		if (IsValid(Item.Pickup) && Item.Pickup->IsTransportLocked())
		{
			Item.Pickup->Destroy();
		}
	}
	InputTransportItems.Reset();
	OutputTransportItems.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MachineProcessingTimerHandle);
	}
	bIsProcessing = false;

	Super::EndPlay(EndPlayReason);
}

void AOreProcessorMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateRollerMotion(DeltaSeconds);
	UpdateInputTransport(DeltaSeconds);
	UpdateOutputTransport(DeltaSeconds);
	RefreshMachineState();
	RefreshTickEnabled();
}

EItemReceiverType AOreProcessorMachine::GetItemReceiverType_Implementation() const
{
	return EItemReceiverType::Processor;
}

bool AOreProcessorMachine::CanAcceptItem_Implementation(const FItemStack& Item) const
{
	return Item.ItemType == EItemType::IronOre
		&& Item.IsValid()
		&& CachedInputPoint
		&& InputSpline
		&& GetBufferedOreCount() + Item.Amount <= MaxBufferedInputOre;
}

FTransform AOreProcessorMachine::GetDeliveryPointWorldTransform() const
{
	if (!CachedInputPoint)
	{
		return GetActorTransform();
	}
	return CachedInputPoint->GetComponentTransform();
}

bool AOreProcessorMachine::AcceptItem_Implementation(const FItemStack& Item)
{
	if (!CanAcceptItem_Implementation(Item))
	{
		return false;
	}

	// InputPoint is the AI parking point. The first point of the authored input
	// spline is the visual ore entry inside the receiving bin.
	FTransform StartTransform = InputSpline->GetTransformAtDistanceAlongSpline(
		0.0f, ESplineCoordinateSpace::World);
	StartTransform.SetScale3D(FVector::OneVector);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	float ReleaseTime = FMath::Max(Now, NextInputReleaseTimeSeconds);
	TArray<AItemPickup*> SpawnedPickups;
	SpawnedPickups.Reserve(Item.Amount);

	for (int32 Index = 0; Index < Item.Amount; ++Index)
	{
		FItemStack UnitStack;
		UnitStack.ItemType = EItemType::IronOre;
		UnitStack.Amount = 1;
		AItemPickup* Pickup = SpawnTransportPickup(
			UnitStack,
			InputOreMesh,
			InputOrePickupClass,
			StartTransform);
		if (!Pickup)
		{
			for (AItemPickup* SpawnedPickup : SpawnedPickups)
			{
				SpawnedPickup->Destroy();
			}
			return false;
		}
		SpawnedPickups.Add(Pickup);
	}

	for (int32 Index = 0; Index < SpawnedPickups.Num(); ++Index)
	{
		FOreProcessorTransportItem& Transport = InputTransportItems.AddDefaulted_GetRef();
		Transport.Pickup = SpawnedPickups[Index];
		Transport.Stack.ItemType = EItemType::IronOre;
		Transport.Stack.Amount = 1;
		Transport.Distance = 0.0f;
		Transport.ReleaseTimeSeconds = ReleaseTime;
		ReleaseTime += InputReleaseInterval;
	}
	NextInputReleaseTimeSeconds = ReleaseTime;

	RefreshMachineState();
	RefreshTickEnabled();
	return true;
}

void AOreProcessorMachine::StartProcessingIfReady()
{
	if (bIsProcessing || QueuedOreCount <= 0)
	{
		return;
	}

	bIsProcessing = true;
	RefreshMachineState();
	RefreshTickEnabled();

	if (!GetWorld() || ProcessingTime <= 0.0f)
	{
		CompleteMachineProcessing();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		MachineProcessingTimerHandle,
		this,
		&AOreProcessorMachine::CompleteMachineProcessing,
		ProcessingTime,
		false);
}

USplineComponent* AOreProcessorMachine::FindAuthoredSpline(FName ComponentName) const
{
	TInlineComponentArray<USplineComponent*> Splines(this);
	for (USplineComponent* Spline : Splines)
	{
		if (Spline && Spline->GetFName() == ComponentName)
		{
			return Spline;
		}
	}
	return nullptr;
}

USceneComponent* AOreProcessorMachine::FindAuthoredSceneComponent(FName ComponentName) const
{
	TInlineComponentArray<USceneComponent*> SceneComponents(this);
	for (USceneComponent* Component : SceneComponents)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}
	return nullptr;
}

AItemPickup* AOreProcessorMachine::SpawnTransportPickup(
	const FItemStack& Stack,
	UStaticMesh* Mesh,
	TSubclassOf<AItemPickup> PickupClass,
	const FTransform& SpawnTransform)
{
	if (!GetWorld() || !Mesh)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* SpawnClass = PickupClass.Get();
	if (!SpawnClass)
	{
		SpawnClass = AItemPickup::StaticClass();
	}
	// Spline transforms can inherit scale from the placed processor Blueprint.
	// A pickup owns the one shared world-size rule, so machine transforms stay unit scale.
	FTransform NormalizedSpawnTransform = SpawnTransform;
	NormalizedSpawnTransform.SetScale3D(FVector::OneVector);
	AItemPickup* Pickup = GetWorld()->SpawnActor<AItemPickup>(
		SpawnClass, NormalizedSpawnTransform, SpawnParameters);
	if (!Pickup)
	{
		return nullptr;
	}

	TArray<TObjectPtr<UStaticMesh>> Meshes;
	Meshes.Add(Mesh);
	Pickup->InitializeItem(Stack, Meshes);
	Pickup->SetTransportLocked(true);
	return Pickup;
}

void AOreProcessorMachine::UpdateInputTransport(float DeltaSeconds)
{
	if (!InputSpline)
	{
		return;
	}

	const float SplineLength = InputSpline->GetSplineLength();
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	for (int32 Index = InputTransportItems.Num() - 1; Index >= 0; --Index)
	{
		FOreProcessorTransportItem& Item = InputTransportItems[Index];
		if (!IsValid(Item.Pickup))
		{
			InputTransportItems.RemoveAt(Index);
			continue;
		}

		if (Item.bWaitingAtInput)
		{
			continue;
		}

		if (Now + KINDA_SMALL_NUMBER < Item.ReleaseTimeSeconds)
		{
			// Keep unreleased ore visibly staged inside the receiving bin instead
			// of stacking every pending batch at exactly the same transform.
			const FTransform InputPointTransform = InputSpline->GetTransformAtDistanceAlongSpline(
				0.0f, ESplineCoordinateSpace::World);
			const FVector Start = InputPointTransform.GetLocation();
			const FVector Right = InputPointTransform.GetRotation().GetRightVector();
			const FVector Up = InputPointTransform.GetRotation().GetUpVector();
			const int32 Slot = FMath::Max(Index, 0);
			const float SideOffset = static_cast<float>((Slot % 3) - 1) * WaitingOreSpacing * 0.65f;
			const float HeightOffset = static_cast<float>(Slot / 3) * WaitingOreSpacing * 0.45f;
			Item.Pickup->SetActorLocation(Start + Right * SideOffset + Up * HeightOffset);
			continue;
		}

		Item.Distance = FMath::Min(Item.Distance + InputOreTravelSpeed * DeltaSeconds, SplineLength);
		const float VisibleDistance = FMath::Max(Item.Distance, 0.0f);
		Item.Pickup->SetActorLocationAndRotation(
			InputSpline->GetLocationAtDistanceAlongSpline(VisibleDistance, ESplineCoordinateSpace::World),
			InputSpline->GetQuaternionAtDistanceAlongSpline(VisibleDistance, ESplineCoordinateSpace::World));

		if (Item.Distance >= SplineLength - KINDA_SMALL_NUMBER)
		{
			if (!IsProcessingQueueFull())
			{
				AdmitOreAtProcessInput(Index);
			}
			else
			{
				Item.bWaitingAtInput = true;
				Item.Pickup->SetActorLocation(GetWaitingOreLocation(Index));
			}
		}
	}
}

void AOreProcessorMachine::UpdateOutputTransport(float DeltaSeconds)
{
	if (!OutputSpline)
	{
		return;
	}

	const float SplineLength = OutputSpline->GetSplineLength();
	for (int32 Index = OutputTransportItems.Num() - 1; Index >= 0; --Index)
	{
		FOreProcessorTransportItem& Item = OutputTransportItems[Index];
		if (!IsValid(Item.Pickup))
		{
			OutputTransportItems.RemoveAt(Index);
			continue;
		}

		Item.Distance = FMath::Min(Item.Distance + OutputItemTravelSpeed * DeltaSeconds, SplineLength);
		Item.Pickup->SetActorLocationAndRotation(
			OutputSpline->GetLocationAtDistanceAlongSpline(Item.Distance, ESplineCoordinateSpace::World),
			OutputSpline->GetQuaternionAtDistanceAlongSpline(Item.Distance, ESplineCoordinateSpace::World));

		if (Item.Distance >= SplineLength - KINDA_SMALL_NUMBER)
		{
			if (CachedOutputPoint)
			{
				FTransform ReleaseTransform = CachedOutputPoint->GetComponentTransform();
				ReleaseTransform.SetScale3D(FVector::OneVector);
				Item.Pickup->SetActorTransform(ReleaseTransform);
				Item.Pickup->ReleaseStationaryForCollection();
			}
			else
			{
				Item.Pickup->SetTransportLocked(false);
			}
			Item.Pickup->SetOwner(nullptr);
			OutputTransportItems.RemoveAt(Index);
		}
	}
}

void AOreProcessorMachine::AdmitOreAtProcessInput(int32 TransportIndex)
{
	if (!InputTransportItems.IsValidIndex(TransportIndex) || IsProcessingQueueFull())
	{
		return;
	}

	FOreProcessorTransportItem Item = InputTransportItems[TransportIndex];
	if (IsValid(Item.Pickup))
	{
		Item.Pickup->Destroy();
	}
	InputTransportItems.RemoveAt(TransportIndex);
	QueuedOreCount = FMath::Min(QueuedOreCount + 1, ProcessingQueueCapacity);
	OnProcessorQueueChanged.Broadcast(QueuedOreCount, ProcessingQueueCapacity);
	StartProcessingIfReady();
}

void AOreProcessorMachine::TryAdmitWaitingOre()
{
	if (IsProcessingQueueFull())
	{
		return;
	}

	for (int32 Index = 0; Index < InputTransportItems.Num(); ++Index)
	{
		if (InputTransportItems[Index].bWaitingAtInput)
		{
			AdmitOreAtProcessInput(Index);
			return;
		}
	}
}

void AOreProcessorMachine::CompleteMachineProcessing()
{
	if (!bIsProcessing)
	{
		return;
	}

	bIsProcessing = false;
	QueuedOreCount = FMath::Max(QueuedOreCount - 1, 0);
	OnProcessorQueueChanged.Broadcast(QueuedOreCount, ProcessingQueueCapacity);

	if (!SpawnOutputIngot())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[OreProcessor] Failed to spawn output iron ingot. Processor=%s"),
			*GetNameSafe(this));
	}

	TryAdmitWaitingOre();
	RefreshMachineState();
	RefreshTickEnabled();
}

bool AOreProcessorMachine::SpawnOutputIngot()
{
	if (!OutputSpline)
	{
		return false;
	}

	FItemStack IngotStack;
	IngotStack.ItemType = EItemType::IronIngot;
	IngotStack.Amount = FMath::Max(OutputIngotAmount, 1);
	const FTransform StartTransform(
		OutputSpline->GetQuaternionAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World),
		OutputSpline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World));
	AItemPickup* IngotPickup = SpawnTransportPickup(
		IngotStack,
		OutputIngotMesh,
		OutputPickupClass,
		StartTransform);
	if (!IngotPickup)
	{
		return false;
	}

	FOreProcessorTransportItem& Transport = OutputTransportItems.AddDefaulted_GetRef();
	Transport.Pickup = IngotPickup;
	Transport.Stack = IngotStack;
	Transport.Distance = 0.0f;
	return true;
}

void AOreProcessorMachine::RefreshMachineState()
{
	EOreProcessorMachineState NewState = EOreProcessorMachineState::Idle;
	if (bIsProcessing)
	{
		NewState = EOreProcessorMachineState::Processing;
	}
	else if (OutputTransportItems.Num() > 0)
	{
		NewState = EOreProcessorMachineState::TransportingOutput;
	}
	else if (InputTransportItems.ContainsByPredicate([](const FOreProcessorTransportItem& Item) { return Item.bWaitingAtInput; }))
	{
		NewState = EOreProcessorMachineState::WaitingForQueue;
	}
	else if (InputTransportItems.Num() > 0)
	{
		NewState = EOreProcessorMachineState::TransportingInput;
	}

	if (NewState != MachineState)
	{
		MachineState = NewState;
		OnMachineStateChanged.Broadcast(MachineState);
	}

	// Conveyor/rollers and the core light intentionally have independent timing.
	// Refresh on every state evaluation because queue/transport contents can
	// change without changing the broad public MachineState enum.
	UpdateMachineVisualState();
}

void AOreProcessorMachine::RefreshTickEnabled()
{
	SetActorTickEnabled(bIsProcessing || InputTransportItems.Num() > 0 || OutputTransportItems.Num() > 0);
}

void AOreProcessorMachine::UpdateMachineVisualState()
{
	static const FName ConveyorMaterialParameter(TEXT("BeltSpeed"));
	static const FName CorePulseMaterialParameter(TEXT("PulseAmplitude"));
	static const FName ConveyorMeshNames[] = { TEXT("StaticMesh1"), TEXT("StaticMesh4") };
	static const FName CoreMeshName(TEXT("StaticMesh"));
	static constexpr float ActiveConveyorSpeed = -1.35f;
	static constexpr float ActiveCorePulseAmplitude = 5.5f;
	const bool bInputTransportActive = InputTransportItems.ContainsByPredicate(
		[](const FOreProcessorTransportItem& Item) { return !Item.bWaitingAtInput; });
	const bool bCorePulseActive = QueuedOreCount > 0;

	TInlineComponentArray<UStaticMeshComponent*> MeshComponents(this);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const FName ComponentName = MeshComponent->GetFName();
		const bool bIsConveyor = ComponentName == ConveyorMeshNames[0] || ComponentName == ConveyorMeshNames[1];
		const bool bIsCore = ComponentName == CoreMeshName;
		if (!bIsConveyor && !bIsCore)
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(0));
		if (!DynamicMaterial)
		{
			DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(0);
		}
		if (!DynamicMaterial)
		{
			continue;
		}

		if (bIsConveyor)
		{
			DynamicMaterial->SetScalarParameterValue(
				ConveyorMaterialParameter, bInputTransportActive ? ActiveConveyorSpeed : 0.0f);
		}
		else
		{
			DynamicMaterial->SetScalarParameterValue(
				CorePulseMaterialParameter, bCorePulseActive ? ActiveCorePulseAmplitude : 0.0f);
		}
	}
}

void AOreProcessorMachine::UpdateRollerMotion(float DeltaSeconds)
{
	// The locations and child offsets live on the Blueprint CDO. These four
	// constants only describe the two angled shaft directions and their shared
	// visual speed, replacing the former per-frame Blueprint node chain.
	static const FName BottomPivotName(TEXT("RollerPivot_Bottom"));
	static const FName TopPivotName(TEXT("RollerPivot_Top"));
	static const FVector BottomLocalAxis(-0.540f, 0.841f, -0.020f);
	static const FVector TopLocalAxis(-0.508f, 0.860f, -0.055f);
	static constexpr float RollerSpeedDegreesPerSecond = -220.0f;

	const bool bInputTransportActive = InputTransportItems.ContainsByPredicate(
		[](const FOreProcessorTransportItem& Item) { return !Item.bWaitingAtInput; });
	if (DeltaSeconds <= 0.0f || !bInputTransportActive)
	{
		return;
	}

	const float AngleRadians = FMath::DegreesToRadians(RollerSpeedDegreesPerSecond * DeltaSeconds);
	if (USceneComponent* BottomPivot = FindAuthoredSceneComponent(BottomPivotName))
	{
		BottomPivot->AddLocalRotation(
			FQuat(BottomLocalAxis.GetSafeNormal(), AngleRadians), false, nullptr, ETeleportType::TeleportPhysics);
	}
	if (USceneComponent* TopPivot = FindAuthoredSceneComponent(TopPivotName))
	{
		TopPivot->AddLocalRotation(
			FQuat(TopLocalAxis.GetSafeNormal(), AngleRadians), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

int32 AOreProcessorMachine::GetBufferedOreCount() const
{
	int32 Buffered = QueuedOreCount;
	for (const FOreProcessorTransportItem& Item : InputTransportItems)
	{
		Buffered += FMath::Max(Item.Stack.Amount, 0);
	}
	return Buffered;
}

FVector AOreProcessorMachine::GetWaitingOreLocation(int32 WaitingIndex) const
{
	if (!InputSpline)
	{
		return GetActorLocation();
	}
	const float EndDistance = InputSpline->GetSplineLength();
	const FVector End = InputSpline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);
	const FVector Right = InputSpline->GetRightVectorAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);
	const FVector Up = InputSpline->GetUpVectorAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);
	const int32 StackIndex = FMath::Max(WaitingIndex, 0);
	return End + Right * ((StackIndex % 2 == 0 ? -1.0f : 1.0f) * WaitingOreSpacing * 0.45f)
		+ Up * (static_cast<float>(StackIndex / 2) * WaitingOreSpacing * 0.55f);
}
