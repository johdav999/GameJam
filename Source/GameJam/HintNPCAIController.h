#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HintNPCAIController.generated.h"

UCLASS(Blueprintable, BlueprintType)
class GAMEJAM_API AHintNPCAIController : public AAIController
{
    GENERATED_BODY()

public:
    AHintNPCAIController();

    virtual void Tick(float DeltaSeconds) override;
};
