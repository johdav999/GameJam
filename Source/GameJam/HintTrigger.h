#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HintTypes.h"
#include "HintTrigger.generated.h"

class UBoxComponent;
class USoundBase;
struct FTimerHandle;

UCLASS()
class GAMEJAM_API AHintTrigger : public AActor
{
    GENERATED_BODY()

public:
    AHintTrigger();

protected:
    virtual void BeginPlay() override;

    /** Collision component used to detect when the player enters the trigger. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hint", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> TriggerBox;

    /** Unique identifier for the hint that should be granted when activated. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    FName HintID;

    /** Text displayed to the player once the hint is collected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    FText HintText;

    /** Whether the hint persists across loop resets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    bool bIsPersistent;

    /** Temporal state for how the hint should initially be categorized. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    EHintTemporalState TemporalState;

    /** Loop threshold required to unlock a future hint. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    int32 LoopToUnlock;

    /** Optional audio to play immediately when the hint is triggered. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TObjectPtr<USoundBase> TriggerSound;

    /** Optional dialog audio asset paths to play sequentially when the hint is triggered. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TArray<FString> DialogAudio;

    /** Allows the trigger to be used multiple times instead of only once. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hint")
    bool bAllowRetrigger;

    /** Tracks whether the trigger has already activated. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Hint")
    bool bTriggered;

    /** Handle used to sequence dialog playback. */
    FTimerHandle DialogPlaybackHandle;

    /** Index of the dialog audio that is currently being processed. */
    int32 CurrentDialogIndex;

    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** Plays each dialog audio entry sequentially. */
    void PlayNextDialogEntry();

    /** Starts dialog playback from the beginning. */
    void BeginDialogPlayback();

    /** Cancels any active dialog timers. */
    void StopDialogPlayback();

public:
    /** Blueprint event fired whenever the hint trigger successfully activates. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Hint")
    void OnHintTriggered();
};
