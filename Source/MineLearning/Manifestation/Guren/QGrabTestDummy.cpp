#include "QGrabTestDummy.h"
#include "MineLearning/Combat/CombatComponent.h"
#include "MineLearning/Combat/HealthComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"

AQGrabTestDummy::AQGrabTestDummy()
{
	UHealthComponent* Health = CreateDefaultSubobject<UHealthComponent>(TEXT("CombatHealth"));
	Health->Faction = ECombatFaction::Hostile;
	UCombatComponent* Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	Combat->Config = TSoftObjectPtr<UCombatConfig>(FSoftObjectPath(TEXT("/Game/MineLearning/Combat/DA_DummyCombat.DA_DummyCombat")));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HeadAnchor"));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(RootComponent);
	Capsule->InitCapsuleSize(50.f, 150.f);
	Capsule->SetRelativeLocation(FVector(0.f, 0.f, -148.f));
	Capsule->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}
