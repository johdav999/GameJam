#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "WorldDoor.generated.h"

class UStaticMeshComponent;
class UWorldShiftComponent;
class USoundBase;

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

    /** Mesh for the door */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> DoorMesh;

    /** World-shift behavior component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWorldShiftComponent> WorldShiftComponent;

    /** Optional sound when door opens */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> OpenSound;

    /** Optional sound when door closes */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> CloseSound;

    /** Worlds where the door should be visible and solid */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
    TArray<EWorldState> VisibleInWorlds;

    /** Whether the door should auto-play open/close animation on toggle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door Behavior")
    bool bAnimateOnToggle;

private:
    UFUNCTION()
    void HandleWorldShift(EWorldState NewWorld);

    void SetDoorState(bool bShouldBeVisible);
    void PlayDoorAnimation(bool bOpening);

    bool bIsCurrentlyVisible;
    bool bHasInitialized;
};
