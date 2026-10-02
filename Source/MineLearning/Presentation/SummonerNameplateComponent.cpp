#include "SummonerNameplateComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

USummonerNameplateComponent::USummonerNameplateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	HumanPawnClass = TSoftClassPtr<APawn>(FSoftObjectPath(TEXT("/Game/MineLearning/Player/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")));
	WidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MineLearning/GameplayRuntime/UI/WBP_SummonerNameplate.WBP_SummonerNameplate_C")));
}

void USummonerNameplateComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* Player = Cast<APlayerController>(GetOwner());
	if (Player && Player->IsLocalController())
	{
		Player->OnPossessedPawnChanged.AddUniqueDynamic(this, &USummonerNameplateComponent::PawnChanged);
		PawnChanged(nullptr, Player->GetPawn());
	}
}

void USummonerNameplateComponent::PawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	ReleaseNameplate();
	UClass* HumanClass = HumanPawnClass.LoadSynchronous();
	if (!IsValid(NewPawn) || !HumanClass || !NewPawn->IsA(HumanClass))
	{
		return;
	}
	APlayerController* Player = CastChecked<APlayerController>(GetOwner());
	UClass* NameplateClass = WidgetClass.LoadSynchronous();
	if (!NameplateClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SummonerNameplate] Missing widget class."));
		return;
	}
	UUserWidget* Widget = CreateWidget<UUserWidget>(Player, NameplateClass);
	if (!Widget)
	{
		return;
	}
	Nameplate = NewObject<UWidgetComponent>(NewPawn, TEXT("SummonerNameplate"));
	NewPawn->AddInstanceComponent(Nameplate);
	Nameplate->SetupAttachment(NewPawn->GetRootComponent());
	Nameplate->SetRelativeLocation(HeadOffset);
	Nameplate->SetWidgetSpace(EWidgetSpace::Screen);
	Nameplate->SetDrawSize(FVector2D(240.f, 40.f));
	Nameplate->SetPivot(FVector2D(0.5f, 1.f));
	Nameplate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Nameplate->SetOwnerPlayer(Player->GetLocalPlayer());
	Nameplate->SetWidget(Widget);
	Nameplate->RegisterComponent();
}

void USummonerNameplateComponent::ReleaseNameplate()
{
	if (IsValid(Nameplate))
	{
		Nameplate->SetWidget(nullptr);
		Nameplate->DestroyComponent();
	}
	Nameplate = nullptr;
}

void USummonerNameplateComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (APlayerController* Player = Cast<APlayerController>(GetOwner()))
	{
		Player->OnPossessedPawnChanged.RemoveDynamic(this, &USummonerNameplateComponent::PawnChanged);
	}
	ReleaseNameplate();
	Super::EndPlay(Reason);
}
