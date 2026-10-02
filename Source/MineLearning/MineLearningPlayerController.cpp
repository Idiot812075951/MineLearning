#include "MineLearningPlayerController.h"
#include "Combat/AttackStackEffect.h"
#include "Roguelite/RogueliteShop.h"
#include "Roguelite/UpgradeDraftComponent.h"
#include "Roguelite/MineRunCoordinatorComponent.h"
#include "Roguelite/MetaProgressComponent.h"
#include "Roguelite/RunBuildComponent.h"
#include "Roguelite/UpgradeDraftComponent.h"
#include "Combat/AmmoInventoryComponent.h"
#include "AI/PhantomCompanionComponent.h"
#include "Presentation/SummonerNameplateComponent.h"
#include "Demo/DemoRunComponent.h"
#include "Demo/DemoRouteGuide.h"
#include "AI/HaulerCharacter.h"
#include "AI/MiningCompanionCharacter.h"
#include "Mining/ItemLogisticsLibrary.h"
#include "Mining/ResourceCarryComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/CombatDamageSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/UnitEffectComponent.h"
#include "Mining/MineableOre.h"
#include "AI/GunnerCharacter.h"
#include "Manifestation/Guren/GurenQSkillComponent.h"
#include "Manifestation/Guren/QGrabTestDummy.h"
#include "Interaction/GrabbableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"

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
	DemoRun = CreateDefaultSubobject<UDemoRunComponent>(TEXT("DemoRun"));
	CreateDefaultSubobject<UAmmoInventoryComponent>(TEXT("AmmoInventory"));
	CreateDefaultSubobject<UMetaProgressComponent>(TEXT("MetaProgress"));
	CreateDefaultSubobject<URunBuildComponent>(TEXT("RunBuild"));
	CreateDefaultSubobject<UUpgradeDraftComponent>(TEXT("UpgradeDraft"));
	CreateDefaultSubobject<UPhantomCompanionComponent>(TEXT("PhantomCompanion"));
	RunCoordinator = CreateDefaultSubobject<UMineRunCoordinatorComponent>(TEXT("RunCoordinator"));
	CreateDefaultSubobject<USummonerNameplateComponent>(TEXT("SummonerNameplate"));
	DemoWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_DemoRun.WBP_V2_DemoRun_C")));
	CombatDetailsKey = EKeys::I;
	CombatWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_CombatDetails.WBP_V2_CombatDetails_C")));
	WarehouseWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(
		TEXT("/Game/MineLearning/UI/V2/Widgets/WBP_V2_Warehouse.WBP_V2_Warehouse_C")));
}

void AMineLearningPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindKey(CombatDetailsKey, IE_Pressed, this, &AMineLearningPlayerController::ToggleCombatDetails);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AMineLearningPlayerController::SelectCombatUnitUnderCursor).bConsumeInput = false;
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AMineLearningPlayerController::HandleInteraction);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMineLearningPlayerController::CloseMenus);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AMineLearningPlayerController::ToggleDemoTerminal);
	InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AMineLearningPlayerController::ToggleRogueliteMenu);
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AMineLearningPlayerController::OpenTalents);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AMineLearningPlayerController::OpenCodex);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AMineLearningPlayerController::SelectGurenForm);
	InputComponent->BindKey(EKeys::NumPadZero, IE_Pressed, this, &AMineLearningPlayerController::SelectHumanForm);
	InputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &AMineLearningPlayerController::SelectOreBuddyForm);
	InputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &AMineLearningPlayerController::SelectGunnerForm);
	InputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &AMineLearningPlayerController::SelectGurenForm);
}

void AMineLearningPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->RemoveOnActorSpawnedHandler(PawnCameraSpawnHandle);
	DemoRun->OnRunChanged.RemoveDynamic(this, &AMineLearningPlayerController::DemoRunChanged);
	if (UUpgradeDraftComponent* Draft = FindComponentByClass<UUpgradeDraftComponent>())
	{
		Draft->OnOfferChanged.RemoveDynamic(this, &AMineLearningPlayerController::DraftChanged);
	}
	for (const TWeakObjectPtr<ARogueliteShop>& Shop : ObservedShops)
	{
		if (Shop.IsValid()) { Shop->OnRangeChanged.RemoveDynamic(this, &AMineLearningPlayerController::ShopRangeChanged); }
	}
	ObservedShops.Reset();
	if (DemoWidget) { DemoWidget->RemoveFromParent(); }
	DemoWidget = nullptr;
	if (RogueliteWidget) { RogueliteWidget->RemoveFromParent(); }
	RogueliteWidget = nullptr;
	UnbindCombatUnit();
	if (CombatWidget)
	{
		CombatWidget->RemoveFromParent();
	}
	CombatWidget = nullptr;
	CloseWarehouseScreen();
	SetTransformationSelectionOpen(false);
	Super::EndPlay(EndPlayReason);
}

void AMineLearningPlayerController::HandleInteraction()
{
	if (bRogueliteMenuOpen) { CloseRogueliteMenu(); return; }
	if (bDemoTerminalOpen) { ToggleDemoTerminal(); return; }
	if (IsNearRogueliteShop()) { OpenRoguelitePage(ERoguelitePage::Shop); return; }
	if (APawn* ControlledPawn = GetPawn())
	{
		UResourceCarryComponent* Carry = ControlledPawn->FindComponentByClass<UResourceCarryComponent>();
		AActor* Machine = Carry ? UItemLogisticsLibrary::FindNearbyPlayerMachine(ControlledPawn, Carry->GetCurrentItem()) : nullptr;
		if (Machine && !ControlledPawn->IsA<AHaulerCharacter>())
		{
			bool bDelivered = false;
			if (AMiningCompanionCharacter* Buddy = Cast<AMiningCompanionCharacter>(ControlledPawn))
			{
				bDelivered = Buddy->TryDeliverToNearbyMachine();
			}
			else if (ControlledPawn->HasAuthority() && UItemLogisticsLibrary::DeliverItemToReceiver(Machine, Carry->GetCurrentItem()))
			{
				Carry->ClearItems();
				bDelivered = true;
			}
			OnInteractionFeedback.Broadcast(bDelivered
				? NSLOCTEXT("Logistics", "ManualDelivery", "开始交付，无需仓库订单。")
				: NSLOCTEXT("Logistics", "MachineBusy", "暂时无法交付，请等待当前动作结束或设备腾出空间。"));
			return;
		}
	}
	if (bWarehouseScreenOpen)
	{
		CloseWarehouseScreen();
		return;
	}

	if (FindNearbyWarehouse())
	{
		OnInteractionFeedback.Broadcast(NSLOCTEXT("Logistics", "WarehouseTerminal", "仓库管理：Tab；Carrier 装卸货物：鼠标左键。"));
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

void AMineLearningPlayerController::SelectGurenForm()
{
	SelectTransformationForm(EPlayerTransformationForm::Guren);
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
	const bool bMenuOpen = bWarehouseScreenOpen || bTransformationSelectionOpen || bDemoTerminalOpen || bRogueliteMenuOpen || bCombatDetailsOpen;
	if (APawn* ControlledPawn = GetPawn())
	{
		if (bMenuOpen)
		{
			if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(ControlledPawn)) { Gunner->CancelPlayerFire(); }
			ControlledPawn->DisableInput(this);
		}
		else
		{
			ControlledPawn->EnableInput(this);
		}
	}
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(bMenuOpen);
	SetIgnoreLookInput(bMenuOpen);
	if (IsLocalController())
	{
		bShowMouseCursor = bMenuOpen;
		if (bMenuOpen)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
		}
		else
		{
			FInputModeGameOnly InputMode;
			InputMode.SetConsumeCaptureMouseDown(false);
			SetInputMode(InputMode);
		}
	}
}

void AMineLearningPlayerController::BeginPlay()
{
	Super::BeginPlay();
	PawnCameraSpawnHandle = GetWorld()->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &AMineLearningPlayerController::IgnorePawnCameraCollision));
	for (TActorIterator<APawn> It(GetWorld()); It; ++It) { IgnorePawnCameraCollision(*It); }
	if (IsLocalController() && DemoRun->IsEnabled())
	{
		for (TActorIterator<ARogueliteShop> Shop(GetWorld()); Shop; ++Shop)
		{
			ObservedShops.Add(*Shop);
			Shop->OnRangeChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::ShopRangeChanged);
		}
		FindComponentByClass<UUpgradeDraftComponent>()->OnOfferChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::DraftChanged);
		DemoRun->OnRunChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::DemoRunChanged);
		if (UClass* Class = RunMenuWidgetClass.LoadSynchronous())
		{
			RogueliteWidget = CreateWidget<UUserWidget>(this, Class);
			if (RogueliteWidget) { RogueliteWidget->AddToPlayerScreen(80); }
		}
		DemoRoute = GetWorld()->SpawnActor<ADemoRouteGuide>();
		bRogueliteMenuOpen = true;
		if (UClass* Class = DemoWidgetClass.LoadSynchronous())
		{
			DemoWidget = CreateWidget<UUserWidget>(this, Class);
			if (DemoWidget) { DemoWidget->AddToPlayerScreen(60); }
		}
	}
	SelectCombatUnit(GetPawn());
	if (IsLocalController())
	{
		if (UClass* Class = CombatWidgetClass.LoadSynchronous())
		{
			CombatWidget = CreateWidget<UUserWidget>(this, Class);
			if (CombatWidget)
			{
				CombatWidget->AddToPlayerScreen(20);
			}
		}
	}
	RefreshMenuInputState();
}

void AMineLearningPlayerController::IgnorePawnCameraCollision(AActor* Actor)
{
	if (ARogueliteShop* Shop = Cast<ARogueliteShop>(Actor); Shop && IsLocalController())
	{
		ObservedShops.AddUnique(Shop);
		Shop->OnRangeChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::ShopRangeChanged);
	}
	if (!IsValid(Actor) || !Actor->IsA<APawn>()) { return; }
	// Keep spring-arm obstruction tests against the environment, including after transformations/spawns.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
}

void AMineLearningPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	IgnorePawnCameraCollision(InPawn);
	if (DemoRun) { DemoRun->ApplyCoreBonus(InPawn); }
	SelectCombatUnit(InPawn);
	RefreshMenuInputState();
	ShopRangeChanged(InPawn, IsNearRogueliteShop());
}

void AMineLearningPlayerController::ToggleDemoTerminal()
{
	if (!DemoRun || !DemoRun->IsEnabled()) { return; }
	CloseWarehouseScreen();
	SetTransformationSelectionOpen(false);
	bDemoTerminalOpen = !bDemoTerminalOpen;
	if (bDemoTerminalOpen && bRogueliteMenuOpen)
	{
		bRogueliteMenuOpen = false;
		OnRogueliteMenuChanged.Broadcast();
	}
	if (bDemoTerminalOpen && bCombatDetailsOpen)
	{
		bCombatDetailsOpen = false;
		OnCombatInspectionChanged.Broadcast();
	}
	RefreshMenuInputState();
	OnDemoTerminalChanged.Broadcast();
}

void AMineLearningPlayerController::DemoRunChanged()
{
	if (DemoRoute) { DemoRoute->Refresh(this); }
	if (DemoRun->IsFinished() && !bDemoResultPresented)
	{
		bDemoResultPresented = true;
		if (!bDemoTerminalOpen) { ToggleDemoTerminal(); }
	}
}

void AMineLearningPlayerController::ExecuteDemoCommand(EDemoCommand Command)
{
	if (Command == EDemoCommand::Start && DemoRun->IsFinished()) { Command = EDemoCommand::Restart; }
	DemoRun->ExecuteCommand(Command);
	if (bDemoTerminalOpen) { ToggleDemoTerminal(); }
}

void AMineLearningPlayerController::CloseMenus()
{
	if (bCombatDetailsOpen) { ToggleCombatDetails(); }
	if (bRogueliteMenuOpen) { ToggleRogueliteMenu(); }
	if (bDemoTerminalOpen) { ToggleDemoTerminal(); }
	CloseWarehouseScreen();
	SetTransformationSelectionOpen(false);
}

void AMineLearningPlayerController::ToggleRogueliteMenu()
{
	if (bRogueliteMenuOpen) { CloseRogueliteMenu(); }
	else { OpenRoguelitePage(DemoRun && DemoRun->GetPhase() == EDemoPhase::Briefing ? ERoguelitePage::Preparation : ERoguelitePage::Debug); }
}

void AMineLearningPlayerController::OpenRoguelitePage(ERoguelitePage Page)
{
	if (!DemoRun || !DemoRun->IsEnabled()) { return; }
	if ((Page == ERoguelitePage::Shop || Page == ERoguelitePage::Draft) && !IsNearRogueliteShop()) { return; }
	if (Page == ERoguelitePage::Shop && RunCoordinator->GetDraft() && RunCoordinator->GetDraft()->HasPendingOffer()) { Page = ERoguelitePage::Draft; }
	if (bDemoTerminalOpen) { ToggleDemoTerminal(); }
	CloseWarehouseScreen();
	SetTransformationSelectionOpen(false);
	RoguelitePage = Page;
	bRogueliteMenuOpen = true;
	RefreshMenuInputState();
	OnRogueliteMenuChanged.Broadcast();
}

void AMineLearningPlayerController::CloseRogueliteMenu()
{
	bRogueliteMenuOpen = false;
	RefreshMenuInputState();
	OnRogueliteMenuChanged.Broadcast();
}

void AMineLearningPlayerController::OpenTalents() { OpenRoguelitePage(ERoguelitePage::Talents); }
void AMineLearningPlayerController::OpenCodex() { OpenRoguelitePage(ERoguelitePage::Codex); }
bool AMineLearningPlayerController::IsNearRogueliteShop() const { return ARogueliteShop::FindNearby(GetPawn()) != nullptr; }

void AMineLearningPlayerController::ShopRangeChanged(APawn* ChangedPawn, bool bNearby)
{
	if (ChangedPawn != GetPawn()) { return; }
	if (!IsNearRogueliteShop() && bRogueliteMenuOpen && (RoguelitePage == ERoguelitePage::Shop || RoguelitePage == ERoguelitePage::Draft)) { CloseRogueliteMenu(); }
	OnRogueliteMenuChanged.Broadcast();
}

void AMineLearningPlayerController::DraftChanged()
{
	UUpgradeDraftComponent* Draft = RunCoordinator->GetDraft();
	if (bRogueliteMenuOpen && (RoguelitePage == ERoguelitePage::Shop || RoguelitePage == ERoguelitePage::Draft))
	{
		RoguelitePage = Draft && Draft->HasPendingOffer() ? ERoguelitePage::Draft : ERoguelitePage::Shop;
	}
	OnRogueliteMenuChanged.Broadcast();
}

void AMineLearningPlayerController::UnbindCombatUnit()
{
	if (ObservedQHealth.IsValid())
	{
		ObservedQHealth->OnHealthChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
	ObservedQHealth.Reset();
	if (!SelectedCombatUnit)
	{
		return;
	}
	SelectedCombatUnit->OnDestroyed.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitDestroyed);
	if (UHealthComponent* Health = SelectedCombatUnit->FindComponentByClass<UHealthComponent>())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
	if (UCombatComponent* Combat = SelectedCombatUnit->FindComponentByClass<UCombatComponent>())
	{
		Combat->OnAttributesChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
	if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(SelectedCombatUnit))
	{
		Gunner->OnAmmoChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatAmmoChanged);
	}
	if (UGurenQSkillComponent* Q = SelectedCombatUnit->FindComponentByClass<UGurenQSkillComponent>())
	{
		Q->OnTargetsChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
		Q->OnStageChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatQStageChanged);
	}
}

void AMineLearningPlayerController::SelectCombatUnit(AActor* Actor)
{
	if (!IsValid(Actor) || !Actor->FindComponentByClass<UHealthComponent>() || Actor == SelectedCombatUnit)
	{
		return;
	}
	UnbindCombatUnit();
	SelectedCombatUnit = Actor;
	Actor->OnDestroyed.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatUnitDestroyed);
	Actor->FindComponentByClass<UHealthComponent>()->OnHealthChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	if (UCombatComponent* Combat = Actor->FindComponentByClass<UCombatComponent>())
	{
		Combat->OnAttributesChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
	if (AGunnerCharacter* Gunner = Cast<AGunnerCharacter>(Actor))
	{
		Gunner->OnAmmoChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatAmmoChanged);
	}
	if (UGurenQSkillComponent* Q = Actor->FindComponentByClass<UGurenQSkillComponent>())
	{
		Q->OnTargetsChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
		Q->OnStageChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatQStageChanged);
	}
	ObserveQTarget();
	OnCombatInspectionChanged.Broadcast();
}

void AMineLearningPlayerController::SelectCombatUnitUnderCursor()
{
	if (!bCombatDetailsOpen || bWarehouseScreenOpen || bTransformationSelectionOpen || bDemoTerminalOpen || bRogueliteMenuOpen)
	{
		return;
	}
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		SelectCombatUnit(Hit.GetActor());
	}
}

AActor* AMineLearningPlayerController::GetSelectedCombatUnit() const
{
	return IsValid(SelectedCombatUnit) ? SelectedCombatUnit.Get() : GetPawn();
}

void AMineLearningPlayerController::CombatUnitDestroyed(AActor* Actor)
{
	UnbindCombatUnit();
	SelectedCombatUnit = nullptr;
	if (GetPawn() != Actor)
	{
		SelectCombatUnit(GetPawn());
	}
	OnCombatInspectionChanged.Broadcast();
}

void AMineLearningPlayerController::ObserveQTarget()
{
	const UGurenQSkillComponent* Q = SelectedCombatUnit ? SelectedCombatUnit->FindComponentByClass<UGurenQSkillComponent>() : nullptr;
	AActor* Target = Q ? Q->GetTarget() : nullptr;
	if (!Target && Q && Q->GetSelectedTarget())
	{
		Target = Q->GetSelectedTarget()->GetOwner();
	}
	UHealthComponent* Health = Target ? Target->FindComponentByClass<UHealthComponent>() : nullptr;
	if (ObservedQHealth.Get() == Health)
	{
		return;
	}
	if (ObservedQHealth.IsValid())
	{
		ObservedQHealth->OnHealthChanged.RemoveDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
	ObservedQHealth = Health;
	if (Health)
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &AMineLearningPlayerController::CombatUnitChanged);
	}
}

void AMineLearningPlayerController::CombatUnitChanged()
{
	ObserveQTarget();
	OnCombatInspectionChanged.Broadcast();
}

void AMineLearningPlayerController::CombatQStageChanged(EGurenQStage Stage, AActor* Target)
{
	CombatUnitChanged();
}

FText AMineLearningPlayerController::GetControlledSkillDescription(FName SkillId) const
{
	const UCombatComponent* Combat = GetPawn() ? GetPawn()->FindComponentByClass<UCombatComponent>() : nullptr;
	if (Combat)
	{
		for (const FCombatSkillViewData& Skill : Combat->GetPanelData().Skills)
		{
			if (Skill.Spec.SkillId == SkillId)
			{
				return Skill.Description;
			}
		}
	}
	return FText::GetEmpty();
}
void AMineLearningPlayerController::CombatAmmoChanged(int32 Ammo, int32 Maximum) { CombatUnitChanged(); }
void AMineLearningPlayerController::ToggleCombatDetails()
{
	if (bDemoTerminalOpen) { ToggleDemoTerminal(); }
	bCombatDetailsOpen = !bCombatDetailsOpen;
	RefreshMenuInputState();
	OnCombatInspectionChanged.Broadcast();
}

FCombatPanelViewData AMineLearningPlayerController::GetCombatPanelData() const
{
	AActor* Actor = GetSelectedCombatUnit();
	if (const UCombatComponent* Combat = Actor ? Actor->FindComponentByClass<UCombatComponent>() : nullptr)
	{
		return Combat->GetPanelData();
	}
	FCombatPanelViewData Data;
	if (const UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Data.Name = Health->Faction == ECombatFaction::Resource
			? NSLOCTEXT("Combat", "Resource", "矿物 / 资源") : NSLOCTEXT("Combat", "Target", "测试目标");
		Data.Health = Health->GetHealth();
		Data.MaxHealth = Health->GetMaxHealth();
		Data.Details = FText::Format(NSLOCTEXT("Combat", "ResourceHealth", "{0}\n生命 {1} / {2}\n无主动技能。采掘与处决均由统一伤害系统结算。"), Data.Name, FText::AsNumber(Data.Health), FText::AsNumber(Data.MaxHealth));
	}
	return Data;
}

void AMineLearningPlayerController::CombatDamage(float Amount)
{
#if !UE_BUILD_SHIPPING
	const FCombatDamageResult Result = GetWorld()->GetSubsystem<UCombatDamageSubsystem>()->ApplyDebugDamage(GetSelectedCombatUnit(), Amount);
	UE_LOG(LogTemp, Display, TEXT("[Combat GM] Accepted=%d Damage=%.2f HP=%.2f"), Result.bAccepted, Result.AppliedDamage, Result.CurrentHealth);
#endif
}

void AMineLearningPlayerController::BuffAdd(FName Id, float Duration)
{
	URunContentCatalog* Catalog = RunCoordinator->GetCatalog();
	if (!Catalog || !GetPawn() || !FMath::IsFinite(Duration) || Duration < -1.f || Duration > 3600.f) { return; }
	RunCoordinator->AssembleUnit(GetPawn());
	UUnitEffectComponent* Effects = GetPawn()->FindComponentByClass<UUnitEffectComponent>();
	if (!Effects) { return; }
	bool Applied = false;
	for (FName UpgradeId : Catalog->Upgrades->GetRowNames())
	{
		const FUpgradeRow* Row = Catalog->FindUpgrade(UpgradeId);
		for (UUnitEffectDefinition* Definition : Row->Effects)
		{
			if (UpgradeId != Id && Definition->Rule.Id != Id) { continue; }
			UUnitEffectDefinition* Debug = DuplicateObject(Definition, Effects);
			// Stack behaviors must still observe real hits/misses; GM only equips them.
			if (!Debug->IsA<UAttackStackEffectDefinition>())
			{
				Debug->Rule.Trigger = EUnitEffectTrigger::Persistent;
				Debug->Rule.Duration = Duration < 0.f ? Definition->Rule.Duration : Duration;
			}
			else if (Duration > 0.f) { Debug->Rule.Duration = Duration; }
			const FName Source(*FString::Printf(TEXT("GM:%s"), *Definition->Rule.Id.ToString()));
			Effects->RevokeDefinition(Source);
			Applied |= Effects->GrantDefinition(Source, Debug);
		}
	}
	ClientMessage(Applied ? TEXT("GM effect granted; permanent progress unchanged.") : TEXT("Unknown effect, unsupported unit or dead pawn. Use BuffList."));
}

void AMineLearningPlayerController::BuffRemove(FName Id)
{
	UUnitEffectComponent* Effects = GetPawn() ? GetPawn()->FindComponentByClass<UUnitEffectComponent>() : nullptr;
	if (!Effects) { return; }
	for (FName Source : Effects->GetGrantedSources())
	{
		if (Source.ToString().StartsWith(TEXT("GM:")) && Source.ToString().Contains(Id.ToString()))
		{
			Effects->RevokeDefinition(Source);
		}
	}
}

void AMineLearningPlayerController::BuffClear()
{
	if (UUnitEffectComponent* Effects = GetPawn() ? GetPawn()->FindComponentByClass<UUnitEffectComponent>() : nullptr)
	{
		for (FName Source : Effects->GetGrantedSources())
		{
			if (Source.ToString().StartsWith(TEXT("GM:"))) { Effects->RevokeDefinition(Source); }
		}
	}
}

void AMineLearningPlayerController::BuffList()
{
	const URunContentCatalog* Catalog = RunCoordinator->GetCatalog();
	if (!Catalog) { return; }
	for (FName Id : Catalog->Upgrades->GetRowNames()) { ClientMessage(Id.ToString()); }
}

void AMineLearningPlayerController::CombatHeal(float Amount)
{
#if !UE_BUILD_SHIPPING
	if (AActor* Target = GetSelectedCombatUnit())
	{
		if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
		{
			Health->Heal(Amount);
		}
	}
#endif
}

void AMineLearningPlayerController::CombatSetHealth(float DesiredHealth)
{
#if !UE_BUILD_SHIPPING
	if (!FMath::IsFinite(DesiredHealth) || DesiredHealth < 0.f)
	{
		return;
	}
	if (AActor* Target = GetSelectedCombatUnit())
	{
		if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
		{
			if (DesiredHealth < Health->GetHealth())
			{
				CombatDamage(Health->GetHealth() - DesiredHealth);
			}
			else
			{
				Health->Heal(DesiredHealth - Health->GetHealth());
			}
		}
	}
#endif
}

void AMineLearningPlayerController::CombatSelectNearestOre()
{
#if !UE_BUILD_SHIPPING
	const FVector Origin = GetPawn() ? GetPawn()->GetActorLocation() : GetFocalLocation();
	AMineableOre* NearestOre = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AMineableOre> It(GetWorld()); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared(Origin, It->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestOre = *It;
		}
	}
	if (NearestOre)
	{
		SelectCombatUnit(NearestOre);
		UE_LOG(LogTemp, Display, TEXT("[Combat GM] Selected nearest ore %s (%.0f cm)"), *NearestOre->GetName(), FMath::Sqrt(NearestDistanceSquared));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Combat GM] No ore found in the current world."));
	}
#endif
}

void AMineLearningPlayerController::CombatSpawnDummy(float MaximumHealth)
{
#if !UE_BUILD_SHIPPING
	if (!HasAuthority() || !GetPawn() || !FMath::IsFinite(MaximumHealth) || MaximumHealth <= 0.f)
	{
		return;
	}
	const FVector Location = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * 400.f;
	AActor* Dummy = GetWorld()->SpawnActor<AActor>();
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Dummy);
	Dummy->SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->RegisterComponent();
	Dummy->SetActorLocation(Location);
	Dummy->SetActorScale3D(FVector(1.5f));
	UHealthComponent* Health = NewObject<UHealthComponent>(Dummy);
	Health->RegisterComponent();
	Health->InitializeHealth(MaximumHealth);
	UGrabbableComponent* Grab = NewObject<UGrabbableComponent>(Dummy);
	Grab->SetupAttachment(Mesh);
	Grab->RegisterComponent();
	SelectCombatUnit(Dummy);
	UE_LOG(LogTemp, Display, TEXT("[Combat GM] Selected test target %s, HP %.1f. CombatSetHealth / CombatDamage / CombatHeal operate on it."), *Dummy->GetName(), MaximumHealth);
#endif
}
