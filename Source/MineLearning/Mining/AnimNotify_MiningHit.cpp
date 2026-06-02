// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_MiningHit.h"

#include "MineLearning/MineLearningCharacter.h"

void UAnimNotify_MiningHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}
	//TODO 这里要改，不能在表现层写业务
	AMineLearningCharacter* Character = Cast<AMineLearningCharacter>(Owner);
	if (!Character)
	{
		return;
	}

	Character->OnMiningHitNotify();
}
