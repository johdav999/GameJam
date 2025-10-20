#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HintNPCCharacter.generated.h"

class UAnimationAsset;
class UAnimInstance;
class AHintNPCAIController;

UCLASS(Blueprintable, BlueprintType)
class GAMEJAM_API AHintNPCCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AHintNPCCharacter();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    UAnimationAsset* AnimationAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    TSubclassOf<UAnimInstance> AnimationBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HintNPC")
    bool bUseAnimationBlueprint;
};
