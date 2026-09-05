#include "SellStation.h"

#include "ItemLogisticsLibrary.h"
#include "ItemPickup.h"
#include "ItemTypes.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace SellStationPresentation
{
	static const FName ActiveSaleTag(TEXT("SellPresentation.Active"));
	// Match the authored scan choreography: the resource remains readable long
	// enough for the player to understand the transaction before payout appears.
	constexpr float ScanDuration = 2.20f;
	constexpr float CoinPopDuration = 0.28f;
	constexpr float CoinObservationDuration = 1.60f;
	constexpr float CoinPopScale = 1.20f;
	constexpr float CoinTimerInterval = 1.0f / 60.0f;
	constexpr int32 MaxDisplayedSaleItems = 5;
	constexpr float SaleItemGapCm = 2.0f;

	FTransform MakeSaleItemTransform(
		int32 Index,
		int32 VisibleCount,
		const FVector& ItemWorldSize,
		const FVector& ParentWorldScale)
	{
		const float BaseHeight = ItemWorldSize.Z * 0.5f + 1.0f;
		FVector WorldOffset(0.0f, 0.0f, BaseHeight);
		if (VisibleCount == 2)
		{
			WorldOffset.X = Index == 0 ? -18.0f : 18.0f;
		}
		else if (VisibleCount == 3)
		{
			static const FVector TriangleOffsets[] =
			{
				FVector(-18.0f, -12.0f, 0.0f),
				FVector(18.0f, -12.0f, 0.0f),
				FVector(0.0f, 16.0f, 0.0f)
			};
			WorldOffset += TriangleOffsets[Index];
		}
		else if (VisibleCount >= 4)
		{
			static const FVector BaseOffsets[] =
			{
				FVector(-18.0f, -14.0f, 0.0f),
				FVector(18.0f, -14.0f, 0.0f),
				FVector(-18.0f, 14.0f, 0.0f),
				FVector(18.0f, 14.0f, 0.0f)
			};
			if (Index < UE_ARRAY_COUNT(BaseOffsets))
			{
				WorldOffset += BaseOffsets[Index];
			}
			else
			{
				WorldOffset.Z += ItemWorldSize.Z + SaleItemGapCm;
			}
		}

		const FVector SafeParentScale(
			FMath::Max(FMath::Abs(ParentWorldScale.X), UE_SMALL_NUMBER),
			FMath::Max(FMath::Abs(ParentWorldScale.Y), UE_SMALL_NUMBER),
			FMath::Max(FMath::Abs(ParentWorldScale.Z), UE_SMALL_NUMBER));
		const FVector LocalOffset(
			WorldOffset.X / SafeParentScale.X,
			WorldOffset.Y / SafeParentScale.Y,
			WorldOffset.Z / SafeParentScale.Z);
		return FTransform(FRotator(0.0f, 17.0f + Index * 23.0f, 0.0f), LocalOffset);
	}
}

ASellStation::ASellStation()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ItemDisplayPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDisplayPoint"));
	ItemDisplayPoint->SetupAttachment(SceneRoot);
	ItemDisplayPoint->SetRelativeLocation(FVector(0.0f, 20.0f, 85.0f));

	TransactionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TransactionPoint"));
	TransactionPoint->SetupAttachment(SceneRoot);
	TransactionPoint->SetRelativeLocation(FVector(0.0f, 20.0f, 85.0f));

	CoinSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CoinSpawnPoint"));
	CoinSpawnPoint->SetupAttachment(SceneRoot);
	CoinSpawnPoint->SetRelativeLocation(FVector(0.0f, 119.0f, 58.0f));

	RobotApproachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RobotApproachPoint"));
	RobotApproachPoint->SetupAttachment(SceneRoot);
	// Keep the shared 85 cm navigation capsule clear of the station's simple collision.
	RobotApproachPoint->SetRelativeLocation(FVector(0.0f, 210.0f, 0.0f));

	StationDisplayName = NSLOCTEXT("MineLearning", "SellStationDisplayName", "出售点");
	CoinPickupClass = AItemPickup::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CoinMeshFinder(
		TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin"));
	if (CoinMeshFinder.Succeeded())
	{
		CoinMesh = CoinMeshFinder.Object;
	}
}

void ASellStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EnforceAuthoredCollisionRoles();
}

void ASellStation::BeginPlay()
{
	Super::BeginPlay();
	EnforceAuthoredCollisionRoles();
}

void ASellStation::EnforceAuthoredCollisionRoles()
{
	// Old World Partition actor records may carry component-instance data from
	// before the presentation pass existed. Reassert the Blueprint/C++ contract
	// both in construction and BeginPlay so every placed station presents alike.
	ItemDisplayPoint->SetRelativeLocation(FVector(0.0f, 20.0f, 85.0f));
	TransactionPoint->SetRelativeLocation(FVector(0.0f, 20.0f, 85.0f));
	CoinSpawnPoint->SetRelativeLocation(FVector(0.0f, 119.0f, 58.0f));
	if (!IsValid(CoinMesh))
	{
		CoinMesh = LoadObject<UStaticMesh>(
			nullptr,
			TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin"));
	}

	UStaticMesh* PresentationOreMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_Drop_01.SM_Ore_Iron_Drop_01"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* CyanMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/MineLearning/Mining/SellStation/M_SS_Cyan.M_SS_Cyan"));
	UMaterialInterface* OrangeMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/MineLearning/Mining/SellStation/M_SS_Orange.M_SS_Orange"));
	UClass* ScreenWidgetClass = StaticLoadClass(
		UUserWidget::StaticClass(),
		nullptr,
		TEXT("/Game/MineLearning/Mining/SellStation/WBP_SellStationScreen.WBP_SellStationScreen_C"));

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetFName() == TEXT("Body"))
		{
			// The authored mesh contains the screen and decorative overhangs. Keep
			// those shapes visual-only so they cannot carve the approach NavMesh.
			Component->SetCollisionProfileName(TEXT("NoCollision"));
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetCanEverAffectNavigation(false);
		}
		else if (Component->GetFName() == TEXT("Screen"))
		{
			Component->SetCollisionProfileName(TEXT("NoCollision"));
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetCanEverAffectNavigation(false);
		}
		else if (Component->GetFName() == TEXT("StationBlocker"))
		{
			// One predictable footprint represents the machine's real base while
			// leaving the delivery and payment lanes clear.
			Component->SetCollisionProfileName(TEXT("BlockAll"));
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Component->SetCanEverAffectNavigation(true);
			Component->SetGenerateOverlapEvents(false);
			Component->SetHiddenInGame(true);
			if (UBoxComponent* Blocker = Cast<UBoxComponent>(Component))
			{
				Blocker->SetBoxExtent(FVector(135.0f, 100.0f, 45.0f));
				Blocker->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
			}
		}
		else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
		{
			const FName ComponentName = StaticMeshComponent->GetFName();
			if (ComponentName == TEXT("PresentationItem"))
			{
				StaticMeshComponent->SetStaticMesh(PresentationOreMesh);
				StaticMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 11.9f));
				StaticMeshComponent->SetRelativeRotation(FRotator(0.0f, 28.0f, 0.0f));
				StaticMeshComponent->SetRelativeScale3D(FVector::OneVector);
			}
			else if (ComponentName == TEXT("ScanPad"))
			{
				StaticMeshComponent->SetStaticMesh(CylinderMesh);
				StaticMeshComponent->SetMaterial(0, CyanMaterial);
				StaticMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.6f));
				StaticMeshComponent->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.006f));
			}
			else if (ComponentName == TEXT("ScanBeam"))
			{
				StaticMeshComponent->SetStaticMesh(CubeMesh);
				StaticMeshComponent->SetMaterial(0, CyanMaterial);
				StaticMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
				StaticMeshComponent->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.008f));
			}
			else if (ComponentName == TEXT("OutputFlash"))
			{
				StaticMeshComponent->SetStaticMesh(SphereMesh);
				StaticMeshComponent->SetMaterial(0, OrangeMaterial);
				StaticMeshComponent->SetRelativeLocation(FVector::ZeroVector);
				StaticMeshComponent->SetRelativeScale3D(FVector(0.12f));
			}
			else
			{
				continue;
			}

			StaticMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticMeshComponent->SetCanEverAffectNavigation(false);
			StaticMeshComponent->SetGenerateOverlapEvents(false);
			StaticMeshComponent->SetCastShadow(false);
			StaticMeshComponent->SetHiddenInGame(true);
		}
		else if (Component->GetFName() == TEXT("ScreenWidget"))
		{
			if (UWidgetComponent* ScreenWidget = Cast<UWidgetComponent>(Component))
			{
				ScreenWidget->SetWidgetClass(ScreenWidgetClass);
				ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
				ScreenWidget->SetDrawSize(FVector2D(1000.0f, 360.0f));
				ScreenWidget->SetDrawAtDesiredSize(false);
				ScreenWidget->SetPivot(FVector2D(0.5f, 0.5f));
				ScreenWidget->SetBlendMode(EWidgetBlendMode::Transparent);
				ScreenWidget->SetTwoSided(false);
				ScreenWidget->SetTickWhenOffscreen(false);
				ScreenWidget->SetManuallyRedraw(false);
				ScreenWidget->SetRelativeLocation(FVector(0.0f, -76.0f, 170.0f));
				ScreenWidget->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
				ScreenWidget->SetRelativeScale3D(FVector(0.168f));
				ScreenWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ScreenWidget->SetCanEverAffectNavigation(false);
				ScreenWidget->SetGenerateOverlapEvents(false);
				ScreenWidget->SetCastShadow(false);
				ScreenWidget->SetHiddenInGame(false);
			}
		}
	}
}

EItemReceiverType ASellStation::GetItemReceiverType_Implementation() const
{
	return EItemReceiverType::SellPoint;
}

bool ASellStation::CanAcceptItem_Implementation(const FItemStack& Item) const
{
	return !ActorHasTag(SellStationPresentation::ActiveSaleTag)
		&& Item.IsValid()
		&& UItemLogisticsLibrary::GetUnitSellPrice(Item.ItemType) > 0;
}

bool ASellStation::AcceptItem_Implementation(const FItemStack& Item)
{
	if (!CanAcceptItem_Implementation(Item))
	{
		return false;
	}

	const int32 UnitPrice = UItemLogisticsLibrary::GetUnitSellPrice(Item.ItemType);
	const int64 CalculatedCoinAmount = static_cast<int64>(Item.Amount) * UnitPrice;
	if (CalculatedCoinAmount <= 0 || CalculatedCoinAmount > MAX_int32)
	{
		return false;
	}
	if (!ConfigureSaleItemStack(Item))
	{
		return false;
	}

	Tags.AddUnique(SellStationPresentation::ActiveSaleTag);
	if (!SpawnCoinPickup(static_cast<int32>(CalculatedCoinAmount)))
	{
		ClearSaleItemStack();
		Tags.Remove(SellStationPresentation::ActiveSaleTag);
		return false;
	}

	OnSaleCompleted.Broadcast(Item, static_cast<int32>(CalculatedCoinAmount));
	return true;
}

bool ASellStation::ConfigureSaleItemStack(const FItemStack& Item)
{
	UStaticMeshComponent* PrimaryItem = FindPresentationItemVisual();
	UInstancedStaticMeshComponent* StackVisual = FindOrCreateSaleItemStackVisual();
	UStaticMesh* ItemMesh = PrimaryItem ? PrimaryItem->GetStaticMesh() : nullptr;
	if (!PrimaryItem || !StackVisual || !IsValid(ItemMesh) || !Item.IsValid())
	{
		return false;
	}

	const int32 VisibleCount = FMath::Clamp(
		Item.Amount,
		1,
		SellStationPresentation::MaxDisplayedSaleItems);
	const FVector ItemWorldSize = MineLearningItemVisual::GetWorldSize(
		ItemMesh,
		Item.ItemType);
	const FVector PrimaryParentScale = PrimaryItem->GetAttachParent()
		? PrimaryItem->GetAttachParent()->GetComponentScale()
		: FVector::OneVector;
	const FTransform PrimaryTransform = SellStationPresentation::MakeSaleItemTransform(
		VisibleCount - 1,
		VisibleCount,
		ItemWorldSize,
		PrimaryParentScale);
	PrimaryItem->SetRelativeLocationAndRotation(
		PrimaryTransform.GetLocation(),
		PrimaryTransform.Rotator());
	PrimaryItem->SetRelativeScale3D(MineLearningItemVisual::GetRelativeScale(
		ItemMesh,
		PrimaryParentScale,
		Item.ItemType));
	PrimaryItem->SetHiddenInGame(false);
	PrimaryItem->SetVisibility(true, true);

	StackVisual->SetStaticMesh(ItemMesh);
	StackVisual->SetMaterial(0, ItemMesh->GetMaterial(0));
	StackVisual->ClearInstances();
	const FVector StackParentScale = StackVisual->GetComponentScale();
	const FVector InstanceScale = MineLearningItemVisual::GetRelativeScale(
		ItemMesh,
		StackParentScale,
		Item.ItemType);
	for (int32 Index = 0; Index < VisibleCount - 1; ++Index)
	{
		FTransform InstanceTransform = SellStationPresentation::MakeSaleItemTransform(
			Index,
			VisibleCount,
			ItemWorldSize,
			StackParentScale);
		InstanceTransform.SetScale3D(InstanceScale);
		StackVisual->AddInstance(InstanceTransform);
	}
	StackVisual->SetHiddenInGame(false);
	StackVisual->SetVisibility(VisibleCount > 1, true);
	return true;
}

void ASellStation::ClearSaleItemStack()
{
	if (UInstancedStaticMeshComponent* StackVisual = FindOrCreateSaleItemStackVisual())
	{
		StackVisual->ClearInstances();
		StackVisual->SetVisibility(false, true);
		StackVisual->SetHiddenInGame(true);
	}
	if (UStaticMeshComponent* PrimaryItem = FindPresentationItemVisual())
	{
		PrimaryItem->SetVisibility(false, true);
		PrimaryItem->SetHiddenInGame(true);
	}
}

UInstancedStaticMeshComponent* ASellStation::FindOrCreateSaleItemStackVisual()
{
	static const FName StackVisualName(TEXT("SaleItemStackVisual"));
	TInlineComponentArray<UInstancedStaticMeshComponent*> InstancedMeshes(this);
	for (UInstancedStaticMeshComponent* InstancedMesh : InstancedMeshes)
	{
		if (InstancedMesh && InstancedMesh->GetFName() == StackVisualName)
		{
			return InstancedMesh;
		}
	}

	if (!ItemDisplayPoint || !GetWorld())
	{
		return nullptr;
	}
	UInstancedStaticMeshComponent* StackVisual = NewObject<UInstancedStaticMeshComponent>(
		this,
		StackVisualName,
		RF_Transient);
	if (!StackVisual)
	{
		return nullptr;
	}

	StackVisual->CreationMethod = EComponentCreationMethod::Instance;
	StackVisual->SetupAttachment(ItemDisplayPoint);
	StackVisual->SetMobility(EComponentMobility::Movable);
	StackVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StackVisual->SetGenerateOverlapEvents(false);
	StackVisual->SetCanEverAffectNavigation(false);
	StackVisual->SetCastShadow(true);
	StackVisual->SetVisibility(false, true);
	StackVisual->SetHiddenInGame(true);
	AddInstanceComponent(StackVisual);
	StackVisual->RegisterComponent();
	return StackVisual;
}

UStaticMeshComponent* ASellStation::FindPresentationItemVisual() const
{
	TInlineComponentArray<UStaticMeshComponent*> StaticMeshes(this);
	for (UStaticMeshComponent* StaticMesh : StaticMeshes)
	{
		if (StaticMesh && StaticMesh->GetFName() == TEXT("PresentationItem"))
		{
			return StaticMesh;
		}
	}
	return nullptr;
}

bool ASellStation::SpawnCoinPickup(int32 CoinAmount)
{
	if (!GetWorld() || CoinAmount <= 0 || !IsValid(CoinMesh))
	{
		return false;
	}

	TSubclassOf<AItemPickup> PickupClassToSpawn = CoinPickupClass;
	if (!PickupClassToSpawn || PickupClassToSpawn == AItemPickup::StaticClass())
	{
		static const TCHAR* CoinPickupBlueprintPath =
			TEXT("/Game/MineLearning/Mining/Resources/Coin/BP_CoinPickup.BP_CoinPickup_C");
		if (UClass* CoinPickupBlueprintClass = StaticLoadClass(
			AItemPickup::StaticClass(), nullptr, CoinPickupBlueprintPath))
		{
			PickupClassToSpawn = CoinPickupBlueprintClass;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AItemPickup* CoinPickup = GetWorld()->SpawnActor<AItemPickup>(
		PickupClassToSpawn.Get(),
		CoinSpawnPoint->GetComponentTransform(),
		SpawnParameters);
	if (!CoinPickup)
	{
		return false;
	}

	FItemStack CoinStack;
	CoinStack.ItemType = EItemType::Coin;
	CoinStack.Amount = CoinAmount;
	TArray<TObjectPtr<UStaticMesh>> CoinMeshes;
	CoinMeshes.Add(CoinMesh);
	CoinPickup->InitializeItem(CoinStack, CoinMeshes);

	// The pickup is the real economic payout, not a decorative duplicate. Keep
	// it unavailable while the terminal presents the sale, then release the
	// same actor into the existing collection/logistics path.
	CoinPickup->SetTransportLocked(true);
	CoinPickup->SetActorHiddenInGame(true);

	const FVector CoinOutletLocation = CoinSpawnPoint->GetComponentLocation();
	// CoinSpawnPoint is the authored source of truth for payout orientation.
	// The coin mesh's local Z axis is its stack axis, matching warehouse coins;
	// applying an extra roll here turned the entire payout stack onto its side.
	const FQuat CoinOutletRotation = CoinSpawnPoint->GetComponentQuat();
	CoinPickup->SetActorLocationAndRotation(CoinOutletLocation, CoinOutletRotation);
	const FVector CoinDisplayScale = CoinPickup->GetActorScale3D();
	CoinPickup->SetActorScale3D(FVector::ZeroVector);

	TWeakObjectPtr<ASellStation> WeakStation(this);
	TWeakObjectPtr<AItemPickup> WeakCoin(CoinPickup);
	UWorld* World = GetWorld();
	const double PresentationStartTime = World->GetTimeSeconds();
	const TSharedRef<FTimerHandle> TimerHandle = MakeShared<FTimerHandle>();
	World->GetTimerManager().SetTimer(
		*TimerHandle,
		FTimerDelegate::CreateLambda(
			[WeakStation,
			 WeakCoin,
			 World,
			 PresentationStartTime,
			 CoinOutletLocation,
			 CoinOutletRotation,
			 CoinDisplayScale,
			 TimerHandle,
			 bSaleStackCleared = false]() mutable
			{
				AItemPickup* ActiveCoin = WeakCoin.Get();
				if (!ActiveCoin)
				{
					World->GetTimerManager().ClearTimer(*TimerHandle);
					if (ASellStation* Station = WeakStation.Get())
					{
						Station->ClearSaleItemStack();
						Station->Tags.Remove(SellStationPresentation::ActiveSaleTag);
					}
					return;
				}

				const float Elapsed = static_cast<float>(
					World->GetTimeSeconds() - PresentationStartTime);
				if (Elapsed < SellStationPresentation::ScanDuration)
				{
					return;
				}
				if (!bSaleStackCleared)
				{
					if (ASellStation* Station = WeakStation.Get())
					{
						Station->ClearSaleItemStack();
					}
					bSaleStackCleared = true;
				}

				ActiveCoin->SetActorHiddenInGame(false);
				const float PopAlpha = FMath::Clamp(
					(Elapsed - SellStationPresentation::ScanDuration)
						/ SellStationPresentation::CoinPopDuration,
					0.0f,
					1.0f);
				const float CoinScale = PopAlpha < 0.65f
					? FMath::Lerp(0.0f, SellStationPresentation::CoinPopScale, PopAlpha / 0.65f)
					: FMath::Lerp(
						SellStationPresentation::CoinPopScale,
						1.0f,
						(PopAlpha - 0.65f) / 0.35f);
				ActiveCoin->SetActorLocationAndRotation(CoinOutletLocation, CoinOutletRotation);
				ActiveCoin->SetActorScale3D(CoinDisplayScale * CoinScale);

				const float ReleaseTime = SellStationPresentation::ScanDuration
					+ SellStationPresentation::CoinPopDuration
					+ SellStationPresentation::CoinObservationDuration;
				if (Elapsed < ReleaseTime)
				{
					return;
				}

				World->GetTimerManager().ClearTimer(*TimerHandle);
				ActiveCoin->SetActorLocationAndRotation(CoinOutletLocation, CoinOutletRotation);
				ActiveCoin->SetActorScale3D(CoinDisplayScale);
				ActiveCoin->ReleaseStationaryForCollection();
				ActiveCoin->SetOwner(nullptr);
				if (ASellStation* Station = WeakStation.Get())
				{
					Station->Tags.Remove(SellStationPresentation::ActiveSaleTag);
				}
			}),
		SellStationPresentation::CoinTimerInterval,
		true);
	return true;
}
