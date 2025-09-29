#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "WorldManager.generated.h"





class UPostProcessComponent;
class USoundMix;

/** Enum describing the three available world states. */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShifted, EWorldState, NewWorld);

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

    /** Broadcast when the world changes. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift")
    FOnWorldShifted OnWorldShifted;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Applies all audiovisual feedback related to the supplied world. */
    void ApplyWorldFeedback(EWorldState NewWorld);

    /** Applies post-process settings for the supplied world. */
    void ApplyPostProcessForWorld(EWorldState NewWorld);

    /** Applies audio snapshot for the supplied world. */
    void ApplyAudioForWorld(EWorldState NewWorld);

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

    /** Optional default mix used when no world specific mix is set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USoundMix> DefaultSoundMix;

    /** Per-world sound mixes to fade to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    TMap<EWorldState, TObjectPtr<USoundMix>> WorldSoundMixes;

    /** Seconds to fade between audio mixes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio", meta = (AllowPrivateAccess = "true"))
    float SoundMixFadeTime;

    /** Starting world configured in the editor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift", meta = (AllowPrivateAccess = "true"))
    EWorldState StartingWorld;

    /** Cached pointer to the currently active sound mix. */
    TWeakObjectPtr<USoundMix> ActiveSoundMix;

    /** Currently active world. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "World Shift", meta = (AllowPrivateAccess = "true"))
    EWorldState CurrentWorld;
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
