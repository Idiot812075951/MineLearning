#include "GurenQAssetSetup.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMeshSocket.h"

void UGurenQAssetSetup::ConfigureSkeleton(USkeleton* Skeleton, FVector GripLocation)
{
#if WITH_EDITOR
	if (!Skeleton)
	{
		return;
	}
	Skeleton->Modify();
	Skeleton->SetSlotGroupName(TEXT("QFullBody"), TEXT("DefaultGroup"));
	USkeletalMeshSocket* Socket = Skeleton->FindSocket(TEXT("Q_GrabHead"));
	if (!Socket)
	{
		Socket = NewObject<USkeletalMeshSocket>(Skeleton, NAME_None, RF_Transactional);
		Socket->SocketName = TEXT("Q_GrabHead");
		Skeleton->Sockets.Add(Socket);
	}
	Socket->BoneName = TEXT("radiant_hand_r");
	Socket->RelativeLocation = GripLocation;
	Socket->RelativeRotation = FRotator::ZeroRotator;
	Socket->RelativeScale = FVector::OneVector;
	Skeleton->MarkPackageDirty();
#endif
}
