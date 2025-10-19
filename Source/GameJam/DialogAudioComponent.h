#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundAttenuation.h"
#include "DialogAudioComponent.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;
class USoundClass;

DECLARE_LOG_CATEGORY_EXTERN(LogDialogAudio, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogLineEvent, USoundBase*, Sound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogQueueFinishedEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogLoadFailedEvent, FSoftObjectPath, AssetPath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogDuckingEvent, bool, bActive, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogSubtitleEvent, const FText&, Subtitle);

/**
 * Reusable component that provides spatialized playback for dialog lines and SFX with optional subtitle support.
 *
 * Attach to any actor, assign attenuation if desired, then call PlayDialogue/QueueDialogue from Blueprint to drive audio.
 * Bind to OnQueueFinished to trigger subsequent gameplay beats when dialog completes.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class GAMEJAM_API UDialogAudioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDialogAudioComponent();

    //~UActorComponent interface
    virtual void OnRegister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Plays a dialogue line using a direct sound reference with optional subtitle support. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void PlayDialogue(USoundBase* Sound, FText Subtitle, float FadeInTime = 0.05f, float StartTime = 0.f, float SubtitleDurationOverride = -1.f);

    /** Plays a sound effect locally on the owning actor. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void PlaySFX(USoundBase* Sound, float FadeInTime = 0.02f, float StartTime = 0.f);

    /** Loads a sound synchronously from a soft reference before playing it. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void PlayBySoftReference(TSoftObjectPtr<USoundBase> Sound, float FadeInTime = 0.05f);

    /** Queues multiple dialogue entries that are played sequentially. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void QueueDialogue(const TArray<TSoftObjectPtr<USoundBase>>& Lines, float GapSeconds = 0.2f);

    /** Stops the currently playing sound, optionally fading out. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void Stop(float FadeOutTime = 0.1f);

    /** Pauses the currently playing sound if one exists. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void Pause();

    /** Resumes a paused sound. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void Resume();

    /** Adjusts the volume multiplier used by the internal audio component. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void SetVolume(float NewVolume);

    /** Adjusts the pitch multiplier used by the internal audio component. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void SetPitch(float NewPitch);

    /** Re-attaches the audio component to the supplied socket if a skeletal mesh is present. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void AttachToSocket(FName SocketName);

    /** Resolves soft references ahead of time without playing them. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    void Preload(const TArray<TSoftObjectPtr<USoundBase>>& Sounds);

    /** Attempts to play dialog associated with the supplied hint identifier. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Hints")
    void PlayHintDialogue(FName HintID);

    /** Returns true when playback is allowed for the supplied tag and updates the cooldown timestamp. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Audio")
    bool PlayIfNotCoolingDown(FName Tag, float CooldownSeconds);

    /** Returns the subtitle that is currently active. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue|Subtitles")
    FText GetCurrentSubtitle() const { return CurrentSubtitle; }

    /** Delegate fired when a new line starts playing. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogLineEvent OnLineStarted;

    /** Delegate fired when a line finishes playback. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogLineEvent OnLineFinished;

    /** Delegate fired when the queued lines have completed. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogQueueFinishedEvent OnQueueFinished;

    /** Delegate fired when a sound fails to load. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogLoadFailedEvent OnLoadFailed;

    /** Delegate fired when ducking should be toggled. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogDuckingEvent OnDuckingChanged;

    /** Delegate fired when subtitles update. */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FDialogSubtitleEvent OnSubtitleChanged;

protected:
    /** Applies editor configurable settings to the internal audio component. */
    void ApplyAudioSettings();

    /** Attempts to play a sound while accounting for culling and queue state. */
    void InternalPlaySound(USoundBase* Sound, float FadeInTime, float StartTime, bool bIsDialogue, float SubtitleDurationOverride = -1.f, const FText& Subtitle = FText(), bool bFromQueue = false);

    /** Returns whether playback should be culled based on listener distance. */
    bool ShouldCullPlayback(const USoundBase* Sound) const;

    /** Calculates the effective max distance for attenuation. */
    float GetEffectiveMaxDistance() const;

    /** Initiates playback for the next queued line. */
    void PlayNextQueuedLine();

    /** Called once the gap timer completes. */
    void HandleQueueGapElapsed();

    /** Clears the active queue state. */
    void ClearQueue();

    /** Clears the subtitle text. */
    void ClearSubtitle();

    /** Enables or disables ducking notifications. */
    void UpdateDucking(bool bShouldDuck);

    /** Updates the attachment of the audio component. */
    void UpdateAttachment();

    /** Handles when the audio component finishes playing. */
    UFUNCTION()
    void HandleAudioFinished();

protected:
    /** Pending looping state that should be applied on the next playback. */
    bool bPendingLoopingState;

    /** Spatialized audio component used for playback. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAudioComponent> AudioComponent;

    /** Optional attenuation asset applied to the audio component. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Attenuation")
    TObjectPtr<USoundAttenuation> AttenuationSettings;

    /** Optional concurrency settings for managing overlapping dialog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Attenuation")
    TObjectPtr<USoundConcurrency> ConcurrencySettings;

    /** Optional sound class override for routing dialog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Attenuation")
    TObjectPtr<USoundClass> SoundClassOverride;

    /** Whether occlusion traces should be enabled for dialog playback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Attenuation")
    bool bEnableOcclusion;

    /** Volume multiplier applied to playback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float VolumeMultiplier;

    /** Pitch multiplier applied to playback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float PitchMultiplier;

    /** Playback priority forwarded to the audio component. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float Priority;

    /** Whether the final queued line should loop once reached. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Queue")
    bool bLoopLast;

    /** Whether the audio component should auto destroy itself when finished. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    bool bAutoDestroyOnFinish;

    /** Enables distance based culling before playback starts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    bool bCullingEnabled;

    /** Multiplier applied to the attenuation max distance for culling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float CullingDistanceMultiplier;

    /** Default max distance used when no attenuation asset is assigned. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float DefaultMaxDistance;

    /** When true, subtitles are broadcast whenever dialog plays. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Subtitles")
    bool bEnableSubtitles;

    /** Amount of ducking to request while dialog is active. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    float DuckingAmount;

    /** Socket name used to attach the audio component when available. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Audio")
    FName AttachSocketName;

private:
    /** Current subtitle text broadcast to listeners. */
    FText CurrentSubtitle;

    /** Cached subtitle clear timer. */
    FTimerHandle SubtitleTimerHandle;

    /** Gap timer used to sequence queued dialog. */
    FTimerHandle QueueGapTimerHandle;

    /** Pending queue of soft referenced sounds. */
    TArray<TSoftObjectPtr<USoundBase>> QueuedLines;

    /** Index of the next queued line to play. */
    int32 QueueIndex;

    /** Gap between queued lines. */
    float QueueGapSeconds;

    /** Whether a queue is currently active. */
    bool bQueueActive;

    /** Tracks whether ducking is currently active. */
    bool bDuckingActive;

    /** Last sound that started playing. */
    TWeakObjectPtr<USoundBase> ActiveSound;

    /** Map of cooldown tags and their last play timestamp. */
    TMap<FName, float> CooldownTimestamps;

    /** Cached fallback attenuation data. */
    FSoundAttenuationSettings FallbackAttenuation;

    /** Resolves a soft reference synchronously and logs failures. */
    USoundBase* ResolveSound(TSoftObjectPtr<USoundBase> Sound);
};

