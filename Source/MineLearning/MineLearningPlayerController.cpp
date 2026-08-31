#include "MineLearningPlayerController.h"

#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

void AMineLearningPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AMineLearningPlayerController::ToggleTransformationSelection);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
	InputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
}

void AMineLearningPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetTransformationSelectionOpen(false);
	Super::EndPlay(EndPlayReason);
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
	if (APawn* ControlledPawn = GetPawn())
	{
		if (bOpen)
		{
			ControlledPawn->DisableInput(this);
		}
		else
		{
			ControlledPawn->EnableInput(this);
		}
	}
	SetIgnoreMoveInput(bOpen);
	SetIgnoreLookInput(bOpen);
	OnTransformationSelectionVisibilityChanged.Broadcast(bOpen);
}
