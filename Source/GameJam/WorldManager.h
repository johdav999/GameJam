#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "WorldManager.generated.h"


class UAudioComponent;
class UPostProcessComponent;
class USoundBase;
class USoundSubmix;
struct FTimerHandle;

/** Enum describing the three available world states. */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShifted, EWorldState, NewWorld);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimedSolidPhaseChanged, bool, bNowSolid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimedSolidPreWarning, bool, bWillBeSolid);

/**
 * Central manager responsible for tracking the active world and applying
 * audiovisual feedback when the world changes.
 */
UCLASS(BlueprintType)
class GAMEJAM_API AWorldManager : public AActor
{
    GENERATED_BODY()

public:
    AWorldManager();

    /** Returns the globally accessible world manager for the provided world. */
    static AWorldManager* Get(UWorld* World);

    /** Returns the currently active world. */
    UFUNCTION(BlueprintPure, Category = "World Shift")
    EWorldState GetCurrentWorld() const { return CurrentWorld; }

    /** Sets the current world to the supplied value. */
    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void SetWorld(EWorldState NewWorld);

    /** Cycles forward or backward depending on the provided direction. */
    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void CycleWorld(int32 Direction);

    /** Convenience wrapper that cycles forward (Light → Shadow → Dream). */
    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void ShiftToNextWorld();

    /** Convenience wrapper that cycles backward (Dream → Shadow → Light). */
    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void ShiftToPreviousWorld();

    /** Restores the active world and player state to the solid world baseline. */
    UFUNCTION(BlueprintCallable, Category = "World Shift|Reset")
    void ResetWorld();

    /** Assigns the checkpoint used when ResetWorld is executed. */
    UFUNCTION(BlueprintCallable, Category = "World Shift|Reset")
    void SetResetCheckpoint(AActor* NewCheckpoint);

    /** Broadcast when the world changes. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift")
    FOnWorldShifted OnWorldShifted;

    /** Broadcast when the global timed solid phase changes between solid and ghost. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift|Timed Solid")
    FOnTimedSolidPhaseChanged OnTimedSolidPhaseChanged;

    /** Broadcast shortly before the global timed solid phase flips. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift|Timed Solid")
    FOnTimedSolidPreWarning OnTimedSolidPreWarning;

    /** Returns whether the global timed solid state is currently solid. */
    UFUNCTION(BlueprintPure, Category = "World Shift|Timed Solid")
    bool IsGlobalTimedSolidSolid() const { return bGlobalTimedSolid; }



protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Applies all audiovisual feedback related to the supplied world. */
    void ApplyWorldFeedback(EWorldState NewWorld);

    /** Applies post-process settings for the supplied world. */
    void ApplyPostProcessForWorld(EWorldState NewWorld);

    /** Applies audio snapshot for the supplied world. */
    void ApplyAudioForWorld(EWorldState NewWorld);

    /** Handles the global timed solid toggle. */
    void HandleGlobalTimedSolidToggle();

    /** Broadcasts the upcoming timed solid phase. */
    void BroadcastPreWarning();

    /** Starts the global timed solid cycle timers. */
    void StartGlobalTimedSolidCycle();

    /** Stops the global timed solid cycle timers. */
    void StopGlobalTimedSolidCycle();


private:
    static TWeakObjectPtr<AWorldManager> ActiveWorldManager;

    /** Ordered helper used for cycling between worlds. */
    static EWorldState GetNextWorld(EWorldState InWorld);
    static EWorldState GetPreviousWorld(EWorldState InWorld);

    /** Component used to apply camera wide post-process effects. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Shift|Visual", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPostProcessComponent> PostProcessComponent;

    /** Configurable post-process settings per world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Visual", meta = (AllowPrivateAccess = "true"))
    TMap<EWorldState, FPostProcessSettings> WorldPostProcessSettings;

    /** Copy of the initial post process settings used as fallback. */
    UPROPERTY(VisibleInstanceOnly, Category = "World Shift|Visual", meta = (AllowPrivateAccess = "true"))
    FPostProcessSettings DefaultPostProcessSettings;

    /** Per-world output submix routing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    TMap<EWorldState, TObjectPtr<USoundSubmix>> WorldSubmixes;

    /** Per-world music assets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    TMap<EWorldState, TObjectPtr<USoundBase>> WorldSongs;

    /** Seconds to fade between world songs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    float MusicFadeTime;

    /** Starting world configured in the editor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift", meta = (AllowPrivateAccess = "true"))
    EWorldState StartingWorld;

    /** Optional default spawn reference used when resetting the world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Reset", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AActor> DefaultResetSpawnPoint;

    /** Runtime checkpoint that overrides the default spawn reference. */
    UPROPERTY(VisibleInstanceOnly, Category = "World Shift|Reset", meta = (AllowPrivateAccess = "true"))
    TWeakObjectPtr<AActor> ActiveResetCheckpoint;

    /** Currently active music component. */
    TWeakObjectPtr<UAudioComponent> ActiveMusicComponent;

    /** Currently active world. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift", meta = (AllowPrivateAccess = "true"))
    EWorldState CurrentWorld;

    /** Duration of each timed solid phase. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Timed Solid", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
    float CycleInterval;

    /** Seconds before the phase change to broadcast the pre-warning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Timed Solid", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float PreWarningTime;

    /** Tracks whether the global timed solid state is currently solid. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift|Timed Solid", meta = (AllowPrivateAccess = "true"))
    bool bGlobalTimedSolid;

    /** Handle for the repeating global timed solid timer. */
    FTimerHandle GlobalTimedSolidHandle;

    /** Handle for the pre-warning timer. */
    FTimerHandle PreWarningHandle;

};

/**
 * Example of routing an input action from a character:
 *
 * void AGameJamCharacter::CycleWorld(const FInputActionValue& Value)
 * {
 *     if (const float AxisValue = Value.Get<float>(); !FMath::IsNearlyZero(AxisValue))
 *     {
 *         if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
 *         {
 *             Manager->CycleWorld(AxisValue > 0.0f ? 1 : -1);
 *         }
 *     }
 * }
 */
