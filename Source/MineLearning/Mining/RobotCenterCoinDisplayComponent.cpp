#include "RobotCenterCoinDisplayComponent.h"

#include "ItemTypes.h"
#include "ResourceStorageComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

namespace RobotCenterCoinDisplay
{
	const FName CoinStorageTag(TEXT("RobotCenter.CoinStorage"));
	const FName PaymentDockName(TEXT("PaymentDock"));
}

URobotCenterCoinDisplayComponent::URobotCenterCoinDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GoldCoinMesh(
		TEXT("/Game/MineLearning/Mining/Resources/Coin/SM_GoldCoin.SM_GoldCoin"));
	CoinMesh = GoldCoinMesh.Object;
}

void URobotCenterCoinDisplayComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	UPrimitiveComponent* PaymentDock = FindPaymentDock();
	CoinStorage = FindCoinStorage();
	if (!Owner || !Owner->GetRootComponent() || !PaymentDock || !CoinStorage || !CoinMesh)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RobotCenter][TEMP] Coin display missing owner, PaymentDock, CoinStorage, or Coin mesh."));
		return;
	}

	CoinPileVisual = NewObject<UInstancedStaticMeshComponent>(
		Owner,
		TEXT("RobotCenterCheckoutCoinPile"),
		RF_Transient);
	Owner->AddInstanceComponent(CoinPileVisual);
	CoinPileVisual->SetStaticMesh(CoinMesh);
	CoinPileVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoinPileVisual->SetGenerateOverlapEvents(false);
	CoinPileVisual->SetCanEverAffectNavigation(false);
	CoinPileVisual->SetMobility(EComponentMobility::Movable);
	CoinPileVisual->SetupAttachment(Owner->GetRootComponent());
	CoinPileVisual->RegisterComponent();

	const FBoxSphereBounds DockBounds = PaymentDock->Bounds;
	CoinPileVisual->SetWorldLocation(
		DockBounds.Origin + FVector(0.0f, 0.0f, DockBounds.BoxExtent.Z + 5.0f));
	CoinPileVisual->SetWorldRotation(FRotator::ZeroRotator);

	CoinStorage->OnStorageChanged.AddDynamic(
		this,
		&URobotCenterCoinDisplayComponent::HandleStorageChanged);
	RefreshDisplay();
}

void URobotCenterCoinDisplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CoinStorage)
	{
		CoinStorage->OnStorageChanged.RemoveDynamic(
			this,
			&URobotCenterCoinDisplayComponent::HandleStorageChanged);
	}

	if (CoinPileVisual)
	{
		CoinPileVisual->DestroyComponent();
		CoinPileVisual = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void URobotCenterCoinDisplayComponent::HandleStorageChanged(int32 StoredOreCount)
{
	RefreshDisplay();
}

void URobotCenterCoinDisplayComponent::RefreshDisplay()
{
	if (!CoinPileVisual || !CoinStorage)
	{
		return;
	}

	CoinPileVisual->ClearInstances();
	const int32 CoinCount = FMath::Min(
		CoinStorage->GetStoredItemAmount(EItemType::Coin),
		MaxVisibleCoins);
	const FVector CoinWorldSize = MineLearningItemVisual::GetWorldSize(CoinMesh);
	const FVector InstanceScale = MineLearningItemVisual::GetRelativeScale(
		CoinMesh,
		CoinPileVisual->GetComponentScale());
	for (int32 Index = 0; Index < CoinCount; ++Index)
	{
		const int32 StackIndex = Index / MineLearningItemVisual::DefaultStackHeight;
		const int32 StackLevel = Index % MineLearningItemVisual::DefaultStackHeight;
		const int32 Column = StackIndex % 4;
		const int32 Row = StackIndex / 4;
		const FVector Location(
			(Column - 1.5f) * 34.0f,
			(Row - 0.5f) * 34.0f,
			CoinWorldSize.Z * 0.5f + StackLevel * (CoinWorldSize.Z + 1.5f));
		const FRotator Rotation(0.0f, Index * 17.0f, 0.0f);
		CoinPileVisual->AddInstance(
			FTransform(Rotation, Location, InstanceScale));
	}

	CoinPileVisual->SetVisibility(CoinCount > 0, true);
}

UResourceStorageComponent* URobotCenterCoinDisplayComponent::FindCoinStorage() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<UResourceStorageComponent*> StorageComponents(Owner);
	for (UResourceStorageComponent* Storage : StorageComponents)
	{
		if (Storage && Storage->ComponentHasTag(RobotCenterCoinDisplay::CoinStorageTag))
		{
			return Storage;
		}
	}
	return nullptr;
}

UPrimitiveComponent* URobotCenterCoinDisplayComponent::FindPaymentDock() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Owner);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive && Primitive->GetFName() == RobotCenterCoinDisplay::PaymentDockName)
		{
			return Primitive;
		}
	}
	return nullptr;
}
