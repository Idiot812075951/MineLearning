// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_MiningHit.h"
#include "MiningToolComponent.h"

void UAnimNotify_MiningHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UMiningToolComponent* MiningComp = Owner->FindComponentByClass<UMiningToolComponent>();
	if (!MiningComp)
	{
		return;
	}

	MiningComp->HandleMiningHitNotify();
}
