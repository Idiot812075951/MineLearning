#include "OreBuddyAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"

void UOreBuddyAnimInstance::TriggerMiningImpact()
{
	// The notify can arrive before this frame's animation update. Use a sentinel so
	// even a very large DeltaSeconds cannot consume the whole recoil before the
	// first post-evaluated pose has displayed it once.
	MiningImpactElapsed = -1.0f;
	MiningImpactAlpha = 1.0f;
}

void UOreBuddyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MiningImpactElapsed < 0.0f)
	{
		MiningImpactElapsed = 0.0f;
		MiningImpactAlpha = 1.0f;
		return;
	}

	if (MiningImpactAlpha <= 0.0f)
	{
		MiningImpactAlpha = 0.0f;
		return;
	}

	MiningImpactElapsed += FMath::Max(DeltaSeconds, 0.0f);
	MiningImpactAlpha = 1.0f - FMath::Clamp(
		MiningImpactElapsed / FMath::Max(MiningRecoilDuration, UE_SMALL_NUMBER),
		0.0f,
		1.0f
	);
}

void UOreBuddyAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	if (MiningImpactAlpha <= 0.0f)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Mesh)
	{
		return;
	}

	const int32 DrillArmIndex = Mesh->GetBoneIndex(DrillArmBoneName);
	const int32 DrillBitIndex = Mesh->GetBoneIndex(DrillBitBoneName);
	TArray<FTransform>& ComponentSpaceTransforms = Mesh->GetEditableComponentSpaceTransforms();
	if (!ComponentSpaceTransforms.IsValidIndex(DrillArmIndex))
	{
		return;
	}

	// Asset audit confirmed the drill points down local -Y, so recoil is local +Y.
	// Work in component space after pose evaluation so 0.8 means 0.8 cm even though
	// the imported rigid skeleton carries a 100x root scale.
	const FVector RecoilDirection = ComponentSpaceTransforms[DrillArmIndex]
		.GetRotation()
		.RotateVector(FVector::YAxisVector)
		.GetSafeNormal();
	const FVector RecoilOffset = RecoilDirection * MiningRecoilDistanceCm * MiningImpactAlpha;

	ComponentSpaceTransforms[DrillArmIndex].AddToTranslation(RecoilOffset);
	if (ComponentSpaceTransforms.IsValidIndex(DrillBitIndex))
	{
		// Component-space child transforms are already resolved at this stage, so move
		// the bit by the same delta to preserve the rigid DrillArm -> DrillBit hierarchy.
		ComponentSpaceTransforms[DrillBitIndex].AddToTranslation(RecoilOffset);
	}
}
