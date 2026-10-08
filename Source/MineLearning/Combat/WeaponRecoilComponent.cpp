#include "WeaponRecoilComponent.h"
#include "CombatComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWeaponRecoilComponent::UWeaponRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SprayPattern = {{0.f, 0.12f}, {0.15f, 0.45f}, {0.4f, 0.8f}, {0.75f, 1.1f},
		{1.15f, 1.6f}, {1.3f, 2.1f}, {0.7f, 2.2f}, {0.f, 2.3f}, {-0.7f, 2.3f}, {-1.4f, 2.4f}};
}

FVector UWeaponRecoilComponent::ApplyShot(FVector Direction)
{
	const int32 Index = FMath::Clamp(FMath::FloorToInt(Heat), 0, FMath::Max(0, SprayPattern.Num() - 1));
	const FVector2D Offset = SprayPattern.IsEmpty() ? FVector2D::ZeroVector : SprayPattern[Index];
	const float Spread = FMath::Max(0.f, RandomSpreadDegrees) * FMath::Min(1.f, 0.2f + Heat / 8.f);
	FRotator Rotation = Direction.Rotation();
	const UCombatComponent* Combat = GetOwner()->FindComponentByClass<UCombatComponent>();
	const float DriftScale = Combat ? FMath::Clamp(1.f + Combat->GetModifiers().BulletDrift, 0.f, 10.f) : 1.f;
	Rotation.Yaw += (Offset.X + FMath::FRandRange(-Spread, Spread)) * DriftScale;
	Rotation.Pitch += FMath::Max(0.f, Offset.Y + FMath::FRandRange(-Spread, Spread)) * DriftScale;
	Heat = FMath::Min(Heat + 1.f, static_cast<float>(FMath::Max(10, SprayPattern.Num())));
	LastShotTime = GetWorld()->GetTimeSeconds();
	LastRecoveryTime = LastShotTime;
	GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this, &UWeaponRecoilComponent::Recover, 0.025f, true);
	OnRecoilChanged.Broadcast();
	return Rotation.Vector();
}

FVector2D UWeaponRecoilComponent::GetCrosshairScale() const
{
	return FVector2D(1.f + FMath::Min(Heat, 10.f) * 0.22f);
}

void UWeaponRecoilComponent::Recover()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const double Start = FMath::Max(LastRecoveryTime, LastShotTime + FMath::Max(0.f, RecoveryDelay));
	LastRecoveryTime = Now;
	if (Now <= Start) { return; }
	Heat = FMath::Max(0.f, Heat - static_cast<float>(Now - Start) * FMath::Max(0.1f, RecoveryPerSecond));
	OnRecoilChanged.Broadcast();
	if (Heat <= 0.f) { GetWorld()->GetTimerManager().ClearTimer(RecoveryTimer); }
}

void UWeaponRecoilComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearTimer(RecoveryTimer);
	Super::EndPlay(Reason);
}
