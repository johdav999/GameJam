#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldState.h"
#include "WorldDoor.generated.h"

class UStaticMeshComponent;
class UWorldShiftBehaviorComponent;
class USoundBase;

UCLASS()
class GAMEJAM_API AWorldDoor : public AActor
{
    GENERATED_BODY()

public:
    AWorldDoor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Door mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> DoorMesh;

    /** Handles world state (Solid, Ghost, Hidden) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    /** Optional open/close sounds */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> DoorSound;

private:
    /** Respond to world-state changes */
    UFUNCTION()
    void OnWorldStateChanged(EWorldMaterialState NewState);

    /** Internal helper to update collision/visibility */
    void UpdateDoorState(EWorldMaterialState NewState);

    /** Tracks the previously applied material state to drive audio cues. */
    EWorldMaterialState LastAppliedState;
};

