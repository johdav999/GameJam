#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldManager.h"
#include "WorldShiftComponent.generated.h"

UCLASS(ClassGroup=(World), Blueprintable, meta=(BlueprintSpawnableComponent))
class GAMEJAM_API UWorldShiftComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorldShiftComponent();

    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    bool bVisibleInLight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    bool bVisibleInShadow = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    bool bVisibleInDream = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    bool bAffectsCollision = true;

protected:
    void UpdateActorState(bool bShouldBeActive) const;
};
