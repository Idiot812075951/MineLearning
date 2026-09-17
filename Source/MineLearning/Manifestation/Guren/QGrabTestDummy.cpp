#include "QGrabTestDummy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"

AQGrabTestDummy::AQGrabTestDummy()
{
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
	GrabStandPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("GrabStandPoint"));
	GrabStandPoint->SetupAttachment(RootComponent);
	GrabStandPoint->SetRelativeLocation(FVector(-160.f, -53.f, -298.f));
	GrabStandPoint->ArrowColor = FColor::Cyan;
	GrabStandPoint->ArrowSize = 2.f;
	GrabStandPoint->SetHiddenInGame(true);
}
