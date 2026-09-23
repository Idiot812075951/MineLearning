#include "DemoRouteGuide.h"

#include "DemoRunComponent.h"
#include "MineLearning/MineLearningPlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADemoRouteGuide::ADemoRouteGuide()
{
	PrimaryActorTick.bCanEverTick = false;
	Segments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Route"));
	SetRootComponent(Segments);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/MineLearning/UI/Demo/MI_DemoRoute.MI_DemoRoute"));
	Segments->SetStaticMesh(Cylinder.Object);
	Segments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Segments->SetCanEverAffectNavigation(false);
	Segments->SetCastShadow(false);
	if (Material.Succeeded()) { Segments->SetMaterial(0, Material.Object); }
}

void ADemoRouteGuide::Refresh(AMineLearningPlayerController* Controller)
{
	Segments->ClearInstances();
	FVector Destination;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !Controller->GetDemoRun()->GetGuidanceDestination(Destination)) { return; }
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Start, End;
	if (!Navigation || !Navigation->ProjectPointToNavigation(Pawn->GetActorLocation(), Start, FVector(250,250,500))
		|| !Navigation->ProjectPointToNavigation(Destination, End, FVector(300,300,500))) { return; }
	UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(GetWorld(), Start.Location, End.Location, Pawn);
	if (!Path || !Path->IsValid() || Path->IsPartial()) { return; }
	// Navigation corners omit height changes inside a corridor. Sample its surface
	// so a long segment across the upper platform and ramp does not sink underground.
	TArray<FVector> SurfacePoints;
	SurfacePoints.Add(Path->PathPoints[0]);
	for (int32 Index = 1; Index < Path->PathPoints.Num(); ++Index)
	{
		const FVector From = Path->PathPoints[Index-1];
		const FVector To = Path->PathPoints[Index];
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist(From, To) / 60.f));
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			FVector Point = FMath::Lerp(From, To, static_cast<float>(Step) / Steps);
			FNavLocation Surface;
			if (Navigation->ProjectPointToNavigation(Point, Surface, FVector(20,20,500)))
			{
				Point.Z = Surface.Location.Z;
			}
			SurfacePoints.Add(Point);
		}
	}
	const FVector Lift(0,0,HeightAboveGround);
	float UntilNextDot = DotSpacing * .5f;
	for (int32 Index = 1; Index < SurfacePoints.Num(); ++Index)
	{
		const FVector From = SurfacePoints[Index-1] + Lift;
		const FVector To = SurfacePoints[Index] + Lift;
		const FVector Delta = To - From;
		const float Length = Delta.Size();
		if (Length < 1.f) { continue; }
		const FVector Tangent = Delta / Length;
		const FVector Side = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
		const FQuat Rotation = FRotationMatrix::MakeFromZ(FVector::CrossProduct(Side, Tangent)).ToQuat();
		while (UntilNextDot < Length)
		{
			Segments->AddInstance(FTransform(Rotation, From+Tangent*UntilNextDot, FVector(DotDiameter/100.f,DotDiameter/100.f,.012f)), true);
			UntilNextDot += FMath::Max(DotSpacing, DotDiameter * 2.f);
		}
		UntilNextDot -= Length;
	}
}
