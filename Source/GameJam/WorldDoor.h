#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "WorldDoor.generated.h"

class UStaticMeshComponent;
class UWorldShiftBehaviorComponent;
class USoundBase;
class AWorldManager;

/**
 * Actor representing a world-dependent door that toggles visibility and collision
 * when the active world changes.
 */
UCLASS()
class GAMEJAM_API AWorldDoor : public AActor
{
    GENERATED_BODY()

public:
    AWorldDoor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Mesh for the door */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> DoorMesh;

    /** World-shift behavior component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWorldShiftBehaviorComponent> WorldShiftBehavior;

    /** Optional sound when door opens */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> OpenSound;

    /** Optional sound when door closes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> CloseSound;

    /** Worlds where the door should be solid (impassable). Other worlds will be passable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    TArray<EWorldState> SolidInWorlds;

    /** Whether the door should auto-play open/close animation on toggle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Behavior")
    bool bAnimateOnToggle;

private:
    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    void SetDoorState(bool bShouldBeSolid, EWorldState CurrentWorld);
    void PlayDoorAnimation(bool bOpening);

    bool IsSolidInWorld(EWorldState World) const;
    void InitializeWorldBehaviors();

    bool bIsCurrentlySolid;
    bool bHasInitialized;
    EWorldState CachedWorldState;

    TWeakObjectPtr<AWorldManager> CachedWorldManager;
    FRotator InitialDoorRotation;
};
