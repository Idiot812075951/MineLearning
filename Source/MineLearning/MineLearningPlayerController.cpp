#include "MineLearningPlayerController.h"

#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemTypes.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"

#if !UE_BUILD_SHIPPING
namespace MineLearningGM
{
	bool ResolveItem(const FString& Name, EItemType& OutItemType, UStaticMesh*& OutMesh)
	{
		const FString NormalizedName = Name.ToLower();
		const TCHAR* MeshPath = nullptr;
		if (NormalizedName == TEXT("ironore") || NormalizedName == TEXT("ore"))
		{
			OutItemType = EItemType::IronOre;
			MeshPath = TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01");
		}
		else if (NormalizedName == TEXT("coin") || NormalizedName == TEXT("gold"))
		{
			OutItemType = EItemType::Coin;
			MeshPath = TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin");
		}
		else if (NormalizedName == TEXT("ironingot") || NormalizedName == TEXT("ingot"))
		{
			OutItemType = EItemType::IronIngot;
			MeshPath = TEXT("/Game/MineLearning/Mining/Resources/IronIngot/SM_IronIngot.SM_IronIngot");
		}
		else if (NormalizedName == TEXT("ammo"))
		{
			OutItemType = EItemType::Ammo;
			MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		}

		OutMesh = MeshPath ? LoadObject<UStaticMesh>(nullptr, MeshPath) : nullptr;
		return IsValid(OutMesh);
	}

	FVector ResolveAimSpawnLocation(UWorld* World)
	{
		if (!World)
		{
			return FVector::ZeroVector;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		if (PlayerController)
		{
			PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}

		const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 10000.0f;
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GMSpawnPickupsAim), false);
		if (PlayerController && PlayerController->GetPawn())
		{
			QueryParams.AddIgnoredActor(PlayerController->GetPawn());
		}
		if (World->LineTraceSingleByChannel(
			Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			return Hit.ImpactPoint + FVector(0.0f, 0.0f, 28.0f);
		}

		if (PlayerController && PlayerController->GetPawn())
		{
			return PlayerController->GetPawn()->GetActorLocation()
				+ PlayerController->GetPawn()->GetActorForwardVector() * 250.0f
				+ FVector(0.0f, 0.0f, 35.0f);
		}
		return FVector(0.0f, 0.0f, 35.0f);
	}

	void SpawnPickups(UWorld* World, const TArray<FString>& Args, const TOptional<FVector>& ExplicitLocation)
	{
		if (!World)
		{
			return;
		}

		const FString ItemName = Args.IsValidIndex(0) ? Args[0] : TEXT("IronOre");
		const int32 Count = FMath::Clamp(Args.IsValidIndex(1) ? FCString::Atoi(*Args[1]) : 5, 1, 50);
		EItemType ItemType = EItemType::IronOre;
		UStaticMesh* ItemMesh = nullptr;
		if (!ResolveItem(ItemName, ItemType, ItemMesh))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GM] Unknown pickup '%s'. Use IronOre, Coin, IronIngot, or Ammo."),
				*ItemName);
			return;
		}

		FVector SpawnCenter = ExplicitLocation.Get(ResolveAimSpawnLocation(World));
		const int32 Columns = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
		TArray<TObjectPtr<UStaticMesh>> ItemMeshes;
		ItemMeshes.Add(ItemMesh);
		int32 SpawnedCount = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Column = Index % Columns;
			const int32 Row = Index / Columns;
			const FVector GridOffset(
				(Column - (Columns - 1) * 0.5f) * 44.0f,
				Row * 44.0f,
				Index * 0.25f);
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AItemPickup* Pickup = World->SpawnActor<AItemPickup>(
				AItemPickup::StaticClass(),
				SpawnCenter + GridOffset,
				FRotator(0.0f, Index * 23.0f, 0.0f),
				SpawnParameters);
			if (!Pickup)
			{
				continue;
			}

			Pickup->InitializeItem(FItemStack{ItemType, 1}, ItemMeshes);
			++SpawnedCount;
		}

		const FString Message = FString::Printf(
			TEXT("GM generated %d x %s at %s"),
			SpawnedCount,
			*ItemName,
			*SpawnCenter.ToCompactString());
		UE_LOG(LogTemp, Display, TEXT("[GM] %s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, Message);
		}
	}

	void SpawnAtAim(const TArray<FString>& Args, UWorld* World)
	{
		SpawnPickups(World, Args, TOptional<FVector>());
	}

	void SpawnAtCoordinates(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 5)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GM] Usage: MineLearning.GM.SpawnPickupsAt Item Count X Y Z"));
			return;
		}
		SpawnPickups(
			World,
			Args,
			FVector(
				FCString::Atof(*Args[2]),
				FCString::Atof(*Args[3]),
				FCString::Atof(*Args[4])));
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnPickupsCommand(
		TEXT("MineLearning.GM.SpawnPickups"),
		TEXT("Spawn loose pickups at the aimed surface. Usage: MineLearning.GM.SpawnPickups [IronOre|Coin|IronIngot|Ammo] [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnAtAim));

	FAutoConsoleCommandWithWorldAndArgs SpawnPickupsAtCommand(
		TEXT("MineLearning.GM.SpawnPickupsAt"),
		TEXT("Spawn loose pickups at a world position. Usage: MineLearning.GM.SpawnPickupsAt Item Count X Y Z"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnAtCoordinates));
}
#endif

AMineLearningPlayerController::AMineLearningPlayerController()
{
	WarehouseWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/MineLearning/Mining/UI/WBP_Warehouse.WBP_Warehouse_C")));
}

void AMineLearningPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AMineLearningPlayerController::HandleInteraction);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMineLearningPlayerController::CloseWarehouseScreen);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
	InputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
}

void AMineLearningPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseWarehouseScreen();
	SetTransformationSelectionOpen(false);
	Super::EndPlay(EndPlayReason);
}

void AMineLearningPlayerController::HandleInteraction()
{
	if (bWarehouseScreenOpen)
	{
		CloseWarehouseScreen();
		return;
	}

	if (AWarehouseDepot* Warehouse = FindNearbyWarehouse())
	{
		OpenWarehouseScreen(Warehouse);
		return;
	}

	ToggleTransformationSelection();
}

AWarehouseDepot* AMineLearningPlayerController::FindNearbyWarehouse() const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return nullptr;
	}

	AWarehouseDepot* NearestWarehouse = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AWarehouseDepot> WarehouseIterator(World);
		WarehouseIterator;
		++WarehouseIterator)
	{
		AWarehouseDepot* Warehouse = *WarehouseIterator;
		if (!IsValid(Warehouse)
			|| Warehouse->IsActorBeingDestroyed()
			|| !Warehouse->IsPlayerInInteractionRange(ControlledPawn))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			ControlledPawn->GetActorLocation(),
			Warehouse->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestWarehouse = Warehouse;
		}
	}
	return NearestWarehouse;
}

bool AMineLearningPlayerController::OpenWarehouseScreen(AWarehouseDepot* Warehouse)
{
	if (!IsLocalController() || !IsValid(Warehouse) || bWarehouseScreenOpen)
	{
		return false;
	}

	UClass* WidgetClass = WarehouseWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Warehouse] WBP_Warehouse could not be loaded."));
		return false;
	}

	SetTransformationSelectionOpen(false);
	ActiveWarehouse = Warehouse;
	ActiveWarehouseWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!ActiveWarehouseWidget)
	{
		ActiveWarehouse = nullptr;
		return false;
	}

	bWarehouseScreenOpen = true;
	ActiveWarehouseWidget->AddToPlayerScreen(50);
	RefreshMenuInputState();

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActiveWarehouseWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	return true;
}

void AMineLearningPlayerController::CloseWarehouseScreen()
{
	if (ActiveWarehouseWidget)
	{
		ActiveWarehouseWidget->RemoveFromParent();
	}
	ActiveWarehouseWidget = nullptr;
	ActiveWarehouse = nullptr;
	bWarehouseScreenOpen = false;
	RefreshMenuInputState();

	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void AMineLearningPlayerController::ToggleTransformationSelection()
{
	if (bTransformationSelectionOpen)
	{
		SetTransformationSelectionOpen(false);
		return;
	}

	if (APlayerTransformZone::IsPlayerOverlappingTransformZone(this))
	{
		SetTransformationSelectionOpen(true);
	}
}

void AMineLearningPlayerController::SelectTransformationForm(const EPlayerTransformationForm Form)
{
	if (!bTransformationSelectionOpen)
	{
		return;
	}

	if (APlayerTransformZone::TrySelectOverlappingForm(this, Form))
	{
		SetTransformationSelectionOpen(false);
	}
}

void AMineLearningPlayerController::SelectHumanForm()
{
	SelectTransformationForm(EPlayerTransformationForm::Human);
}

void AMineLearningPlayerController::SelectOreBuddyForm()
{
	SelectTransformationForm(EPlayerTransformationForm::OreBuddy);
}

void AMineLearningPlayerController::SelectGunnerForm()
{
	SelectTransformationForm(EPlayerTransformationForm::Gunner);
}

void AMineLearningPlayerController::SetTransformationSelectionOpen(const bool bOpen)
{
	if (bTransformationSelectionOpen == bOpen)
	{
		return;
	}

	bTransformationSelectionOpen = bOpen;
	RefreshMenuInputState();
	OnTransformationSelectionVisibilityChanged.Broadcast(bOpen);
}

void AMineLearningPlayerController::RefreshMenuInputState()
{
	const bool bMenuOpen = bWarehouseScreenOpen || bTransformationSelectionOpen;
	if (APawn* ControlledPawn = GetPawn())
	{
		if (bMenuOpen)
		{
			ControlledPawn->DisableInput(this);
		}
		else
		{
			ControlledPawn->EnableInput(this);
		}
	}
	SetIgnoreMoveInput(bMenuOpen);
	SetIgnoreLookInput(bMenuOpen);
}
