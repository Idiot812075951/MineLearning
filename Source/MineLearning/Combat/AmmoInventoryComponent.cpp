#include "AmmoInventoryComponent.h"

UAmmoInventoryComponent::UAmmoInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UAmmoInventoryComponent::ReserveMagazine()
{
	if (!GetOwner()->HasAuthority() || bReserved || Magazines <= 0)
	{
		return false;
	}
	bReserved = true;
	OnAmmoInventoryChanged.Broadcast();
	return true;
}

bool UAmmoInventoryComponent::CommitMagazine()
{
	if (!bReserved || Magazines <= 0)
	{
		return false;
	}
	bReserved = false;
	--Magazines;
	OnAmmoInventoryChanged.Broadcast();
	return true;
}

void UAmmoInventoryComponent::CancelReservation()
{
	if (bReserved)
	{
		bReserved = false;
		OnAmmoInventoryChanged.Broadcast();
	}
}

void UAmmoInventoryComponent::AddMagazines(int32 Amount)
{
	if (GetOwner()->HasAuthority() && Amount > 0)
	{
		Magazines = static_cast<int32>(FMath::Min<int64>(MAX_int32, static_cast<int64>(Magazines) + Amount));
		OnAmmoInventoryChanged.Broadcast();
	}
}
