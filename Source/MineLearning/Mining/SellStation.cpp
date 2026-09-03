#include "SellStation.h"

#include "ItemLogisticsLibrary.h"
#include "ItemPickup.h"
#include "ItemTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ASellStation::ASellStation()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ItemDisplayPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDisplayPoint"));
	ItemDisplayPoint->SetupAttachment(SceneRoot);
	ItemDisplayPoint->SetRelativeLocation(FVector(0.0f, 7.0f, 85.0f));

	TransactionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TransactionPoint"));
	TransactionPoint->SetupAttachment(SceneRoot);
	TransactionPoint->SetRelativeLocation(FVector(0.0f, 7.0f, 85.0f));

	CoinSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CoinSpawnPoint"));
	CoinSpawnPoint->SetupAttachment(SceneRoot);
	CoinSpawnPoint->SetRelativeLocation(FVector(0.0f, 7.0f, 91.0f));

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

void ASellStation::EnforceAuthoredCollisionRoles()
{
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetFName() == TEXT("Body"))
		{
			Component->SetCollisionProfileName(TEXT("BlockAll"));
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Component->SetCanEverAffectNavigation(true);
		}
		else if (Component->GetFName() == TEXT("Screen"))
		{
			Component->SetCollisionProfileName(TEXT("NoCollision"));
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetCanEverAffectNavigation(false);
		}
	}
}

EItemReceiverType ASellStation::GetItemReceiverType_Implementation() const
{
	return EItemReceiverType::SellPoint;
}

bool ASellStation::CanAcceptItem_Implementation(const FItemStack& Item) const
{
	return Item.IsValid() && UItemLogisticsLibrary::GetUnitSellPrice(Item.ItemType) > 0;
}

bool ASellStation::AcceptItem_Implementation(const FItemStack& Item)
{
	if (!CanAcceptItem_Implementation(Item))
	{
		return false;
	}

	const int32 UnitPrice = UItemLogisticsLibrary::GetUnitSellPrice(Item.ItemType);
	const int64 CalculatedCoinAmount = static_cast<int64>(Item.Amount) * UnitPrice;
	if (CalculatedCoinAmount <= 0 || CalculatedCoinAmount > MAX_int32
		|| !SpawnCoinPickup(static_cast<int32>(CalculatedCoinAmount)))
	{
		return false;
	}

	OnSaleCompleted.Broadcast(Item, static_cast<int32>(CalculatedCoinAmount));
	return true;
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
	CoinPickup->ReleaseStationaryForCollection();
	CoinPickup->SetOwner(nullptr);
	return true;
}
