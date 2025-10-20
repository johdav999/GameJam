#include "HintNPCCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/SkeletalMeshComponent.h"
#include "HintNPCAIController.h"

AHintNPCCharacter::AHintNPCCharacter()
{
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AHintNPCAIController::StaticClass();

    bUseAnimationBlueprint = false;
    AnimationAsset = nullptr;
    AnimationBlueprint = nullptr;
}

void AHintNPCCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (USkeletalMeshComponent* MeshComponent = GetMesh())
    {
        if (bUseAnimationBlueprint && AnimationBlueprint)
        {
            MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
            MeshComponent->SetAnimInstanceClass(AnimationBlueprint);
        }
        else if (AnimationAsset)
        {
            MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            MeshComponent->PlayAnimation(AnimationAsset, true);
        }
        else
        {
            MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            MeshComponent->Stop();
        }
    }
}
