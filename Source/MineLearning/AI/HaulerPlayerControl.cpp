#include "HaulerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "MineLearning/Mining/ItemPickup.h"
#include "MineLearning/Mining/ItemLogisticsLibrary.h"
#include "MineLearning/Mining/ResourceCarryComponent.h"
#include "MineLearning/Mining/WarehouseDepot.h"
#include "MineLearning/Mining/OreProcessorMachine.h"
#include "MineLearning/Mining/SellStation.h"
#include "Components/SceneComponent.h"
#include "MineLearning/AI/MiningCompanionCharacter.h"

FText AHaulerCharacter::GetPlayerTransferFailureReason() const
{
	if (GetWorld()->GetTimeSeconds() < NextPlayerTransferTime)
	{
		return NSLOCTEXT("Logistics", "TransferBusy", "正在装卸，请稍候。");
	}
	const AItemPickup* Nearest = nullptr;
	float Distance = FMath::Square(260.f);
	for (TActorIterator<AItemPickup> It(GetWorld()); It; ++It)
	{
		const float Candidate = FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation());
		if (Candidate < Distance && !It->IsHidden()) { Distance = Candidate; Nearest = *It; }
	}
	if (Nearest)
	{
		if (const AActor* Collector = Nearest->GetReservedCollector(); IsValid(Collector) && Collector != this)
		{
			const FText Name = Collector->IsA<AHaulerCharacter>() ? NSLOCTEXT("Logistics", "CarrierOwner", "自动 Carrier")
				: (Collector->IsA<AMiningCompanionCharacter>() ? NSLOCTEXT("Logistics", "BuddyOwner", "自动 OreBuddy")
				: NSLOCTEXT("Logistics", "OtherOwner", "其他机器人"));
			return FText::Format(NSLOCTEXT("Logistics", "Reserved", "货物已被 {0} 锁定，等待它搬运。"), Name);
		}
		if (Nearest->IsTransportLocked()) { return NSLOCTEXT("Logistics", "Processing", "货物正在设备中运输，请等待产物到达出口。"); }
		if (!ResourceCarryComponent->CanAcceptItem(Nearest->GetItemStack())) { return NSLOCTEXT("Logistics", "CargoMismatch", "货舱已满或货物类型不同，请先交付当前货物。"); }
	}
	return ResourceCarryComponent->IsEmpty()
		? NSLOCTEXT("Logistics", "NoPickup", "附近没有可拾取的货物，请靠近货物后按 E。")
		: NSLOCTEXT("Logistics", "NoDelivery", "暂时无法交付，请靠近引导终点，或等待设备空闲。");
}

void AHaulerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->AddMappingContext(PlayerMapping, 0);
		}
	}
}

void AHaulerCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(Input))
	{
		Enhanced->BindAction(PlayerMoveAction, ETriggerEvent::Triggered, this, &AHaulerCharacter::MovePlayer);
		Enhanced->BindAction(PlayerLookAction, ETriggerEvent::Triggered, this, &AHaulerCharacter::LookPlayer);
	}
}

void AHaulerCharacter::MovePlayer(const FInputActionValue& Value)
{
	if (!Controller) { return; }
	const FVector2D Move = Value.Get<FVector2D>();
	const FRotationMatrix Direction(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	AddMovementInput(Direction.GetUnitAxis(EAxis::X), Move.Y);
	AddMovementInput(Direction.GetUnitAxis(EAxis::Y), Move.X);
}

void AHaulerCharacter::LookPlayer(const FInputActionValue& Value)
{
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

bool AHaulerCharacter::TryPlayerTransfer()
{
	if (!IsPlayerControlled() || !HasAuthority() || GetWorld()->GetTimeSeconds() < NextPlayerTransferTime)
	{
		return false;
	}
	AActor* NearbyMachine = UItemLogisticsLibrary::FindNearbyPlayerMachine(this, ResourceCarryComponent->GetCurrentItem());
	if (NearbyMachine)
	{
		PlayerDeliveryActor = NearbyMachine;
		PlayerDeliveryPoint = nullptr;
	}
	if (!ResourceCarryComponent->IsEmpty() && !IsValid(PlayerDeliveryActor))
	{
		PlayerDeliveryActor = UItemLogisticsLibrary::ResolveDestination(this, ResourceCarryComponent->GetCurrentItem(), GetActorLocation());
	}
	if (!ResourceCarryComponent->IsEmpty() && IsValid(PlayerDeliveryActor))
	{
		FVector Destination;
		GetPlayerDeliveryLocation(Destination);
		if (FVector::DistSquared2D(GetActorLocation(), Destination) <= FMath::Square(260.f)
			&& UItemLogisticsLibrary::DeliverItemToReceiver(PlayerDeliveryActor, ResourceCarryComponent->GetCurrentItem()))
		{
			ResourceCarryComponent->ClearItems();
			HideCarriedItem();
			PlayDropOffAnimation();
			PlayerDeliveryActor = nullptr;
			PlayerDeliveryPoint = nullptr;
			NextPlayerTransferTime = GetWorld()->GetTimeSeconds() + 0.6f;
			return true;
		}
		if (NearbyMachine) { return false; }
	}
	AItemPickup* Best = nullptr;
	float BestDistance = FMath::Square(260.f);
	for (TActorIterator<AItemPickup> It(GetWorld()); It; ++It)
	{
		AItemPickup* Pickup = *It;
		if (!Pickup->IsAvailableFor(this) || !ResourceCarryComponent->CanAcceptItem(Pickup->GetItemStack())) { continue; }
		const float Distance = FVector::DistSquared2D(GetActorLocation(), Pickup->GetActorLocation());
		if (Distance < BestDistance) { Best = Pickup; BestDistance = Distance; }
	}
	if (!Best) { return false; }
	AActor* Route = Best->HasUsableExplicitDeliveryTarget() ? Best->GetExplicitDeliveryActor()
		: UItemLogisticsLibrary::ResolveDestination(this, Best->GetItemStack(), Best->GetActorLocation());
	USceneComponent* Point = Best->GetExplicitDeliveryPoint();
	UStaticMesh* ItemMesh = Best->SelectedDropMesh;
	if (!Best->TryCollect(this)) { return false; }
	PlayerDeliveryActor = Route;
	PlayerDeliveryPoint = Point;
	ShowCarriedItem(ItemMesh);
	PlayPickupAnimation();
	NextPlayerTransferTime = GetWorld()->GetTimeSeconds() + 0.6f;
	return true;
}

bool AHaulerCharacter::GetPlayerDeliveryLocation(FVector& OutLocation) const
{
	if (AActor* Machine = UItemLogisticsLibrary::FindNearbyPlayerMachine(this, ResourceCarryComponent->GetCurrentItem()))
	{
		OutLocation = UItemLogisticsLibrary::GetReceiverDeliveryLocation(Machine);
		return true;
	}
	AActor* Receiver = IsValid(PlayerDeliveryActor) ? PlayerDeliveryActor.Get()
		: UItemLogisticsLibrary::ResolveDestination(this, ResourceCarryComponent->GetCurrentItem(), GetActorLocation());
	if (!IsValid(Receiver) || ResourceCarryComponent->IsEmpty()) { return false; }
	OutLocation = IsValid(PlayerDeliveryPoint) ? PlayerDeliveryPoint->GetComponentLocation() : Receiver->GetActorLocation();
	if (const AWarehouseDepot* Warehouse = Cast<AWarehouseDepot>(Receiver))
	{
		OutLocation = Warehouse->GetDeliveryPointWorldTransform().GetLocation();
	}
	else if (const AOreProcessorMachine* Processor = Cast<AOreProcessorMachine>(Receiver))
	{
		OutLocation = Processor->GetDeliveryPointWorldTransform().GetLocation();
	}
	else if (const ASellStation* Seller = Cast<ASellStation>(Receiver))
	{
		OutLocation = Seller->GetRobotApproachPoint()->GetComponentLocation();
	}
	return true;
}

FText AHaulerCharacter::GetPlayerCargoDescription() const
{
	if (ResourceCarryComponent->IsEmpty())
	{
		return NSLOCTEXT("DemoRun", "CarrierEmptyManual", "Carrier 手动运输 · E 装货 / 卸货\n拾取地上的货物或设备产物，直接送到加工机 / 出售点按 E，无需订单。\n可装 4 件同类货物；仓库订单用于自动搬运或提取库存。" );
	}
	FVector Location = GetActorLocation();
	GetPlayerDeliveryLocation(Location);
	FText Destination = NSLOCTEXT("DemoRun", "DestWarehouse", "仓库入口（上层右侧）");
	const AActor* Receiver = IsValid(PlayerDeliveryActor) ? PlayerDeliveryActor.Get()
		: UItemLogisticsLibrary::ResolveDestination(this, ResourceCarryComponent->GetCurrentItem(), GetActorLocation());
	if (AActor* Machine = UItemLogisticsLibrary::FindNearbyPlayerMachine(this, ResourceCarryComponent->GetCurrentItem())) { Receiver = Machine; }
	if (Cast<AOreProcessorMachine>(Receiver))
	{
		Destination = NSLOCTEXT("DemoRun", "DestProcessorHopper", "加工机入料箱前");
	}
	else if (Cast<ASellStation>(Receiver))
	{
		Destination = NSLOCTEXT("DemoRun", "DestSell", "出售点（左侧坡口）");
	}
	const EItemType Item = ResourceCarryComponent->GetCurrentItem().ItemType;
	const FText ItemName = Item == EItemType::IronOre ? NSLOCTEXT("DemoRun", "CargoOre", "原矿")
		: (Item == EItemType::IronIngot ? NSLOCTEXT("DemoRun", "CargoIngot", "铁锭") : NSLOCTEXT("DemoRun", "CargoCoin", "金币"));
	return FText::Format(NSLOCTEXT("DemoRun", "CarrierLoadedManual", "携带 {0} 件{3}（容量 4，未满也可交付）\n送往：{1} · 约 {2} 米\n也可直接靠近加工机 / 出售点按 E，无需订单。\n配方：2 原矿 → 1 铁锭；2 铁锭 → 4 金币。"),
		FText::AsNumber(ResourceCarryComponent->GetCurrentItemCount()), Destination,
		FText::AsNumber(FMath::RoundToInt(FVector::Dist2D(GetActorLocation(), Location) / 100.f)), ItemName);
}
