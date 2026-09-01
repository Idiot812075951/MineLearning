#include "MineLearningPlayerController.h"

#include "MineLearning/Mining/WarehouseDepot.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

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
