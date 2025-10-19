#include "DialogAudioComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "GameJamGameInstance.h"
#include "HintTypes.h"
//#include "GameFramework/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogDialogAudio);

UDialogAudioComponent::UDialogAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("DialogAudio"));
    if (AudioComponent)
    {
        AudioComponent->bAutoActivate = false;
        AudioComponent->bAllowSpatialization = true;
        AudioComponent->bIsUISound = false;
        AudioComponent->bOverrideAttenuation = true;
        AudioComponent->bAutoDestroy = false;
        AudioComponent->OnAudioFinished.AddDynamic(this, &UDialogAudioComponent::HandleAudioFinished);
    }

    bEnableOcclusion = true;
    VolumeMultiplier = 1.0f;
    PitchMultiplier = 1.0f;
    Priority = 1.0f;
    bLoopLast = false;
    bAutoDestroyOnFinish = false;
    bCullingEnabled = true;
    CullingDistanceMultiplier = 2.0f;
    DefaultMaxDistance = 3000.0f;
    bEnableSubtitles = true;
    DuckingAmount = 0.5f;
    AttachSocketName = NAME_None;

    QueueIndex = 0;
    QueueGapSeconds = 0.2f;
    bQueueActive = false;
    bDuckingActive = false;
    bPendingLoopingState = false;

    FallbackAttenuation = FSoundAttenuationSettings();
    FallbackAttenuation.bAttenuate = true;
    FallbackAttenuation.AttenuationShape = EAttenuationShape::Sphere;
    FallbackAttenuation.AttenuationShapeExtents = FVector(0.f, 0.f, 0.f);
    FallbackAttenuation.FalloffDistance = DefaultMaxDistance;
    FallbackAttenuation.dBAttenuationAtMax = -30.f;
    FallbackAttenuation.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
}

void UDialogAudioComponent::OnRegister()
{
    Super::OnRegister();

    UpdateAttachment();
}

void UDialogAudioComponent::BeginPlay()
{
    Super::BeginPlay();

    ApplyAudioSettings();
}

void UDialogAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearQueue();

    if (AudioComponent)
    {
        AudioComponent->Stop();
        AudioComponent->OnAudioFinished.RemoveDynamic(this, &UDialogAudioComponent::HandleAudioFinished);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SubtitleTimerHandle);
        World->GetTimerManager().ClearTimer(QueueGapTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void UDialogAudioComponent::PlayDialogue(USoundBase* Sound, FText Subtitle, float FadeInTime, float StartTime, float SubtitleDurationOverride)
{
    InternalPlaySound(Sound, FadeInTime, StartTime, true, SubtitleDurationOverride, Subtitle);
}

void UDialogAudioComponent::PlaySFX(USoundBase* Sound, float FadeInTime, float StartTime)
{
    InternalPlaySound(Sound, FadeInTime, StartTime, false);
}

void UDialogAudioComponent::PlayBySoftReference(TSoftObjectPtr<USoundBase> Sound, float FadeInTime)
{
    USoundBase* Resolved = ResolveSound(Sound);
    if (!Resolved)
    {
        return;
    }

    InternalPlaySound(Resolved, FadeInTime, 0.f, true);
}

void UDialogAudioComponent::QueueDialogue(const TArray<TSoftObjectPtr<USoundBase>>& Lines, float GapSeconds)
{
    if (!GetWorld())
    {
        return;
    }

    ClearQueue();

    if (Lines.Num() == 0)
    {
        OnQueueFinished.Broadcast();
        return;
    }

    QueuedLines = Lines;
    QueueIndex = 0;
    QueueGapSeconds = GapSeconds;
    bQueueActive = true;

    PlayNextQueuedLine();
}

void UDialogAudioComponent::Stop(float FadeOutTime)
{
    if (!AudioComponent)
    {
        return;
    }

    if (FadeOutTime > 0.f)
    {
        AudioComponent->FadeOut(FadeOutTime, 0.f);
    }
    else
    {
        AudioComponent->Stop();
        HandleAudioFinished();
    }

    ClearQueue();
}

void UDialogAudioComponent::Pause()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(true);
    }
}

void UDialogAudioComponent::Resume()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(false);
    }
}

void UDialogAudioComponent::SetVolume(float NewVolume)
{
    VolumeMultiplier = NewVolume;
    if (AudioComponent)
    {
        AudioComponent->SetVolumeMultiplier(NewVolume);
    }
}

void UDialogAudioComponent::SetPitch(float NewPitch)
{
    PitchMultiplier = NewPitch;
    if (AudioComponent)
    {
        AudioComponent->SetPitchMultiplier(NewPitch);
    }
}

void UDialogAudioComponent::AttachToSocket(FName SocketName)
{
    AttachSocketName = SocketName;
    UpdateAttachment();
}

void UDialogAudioComponent::Preload(const TArray<TSoftObjectPtr<USoundBase>>& Sounds)
{
    for (const TSoftObjectPtr<USoundBase>& SoftSound : Sounds)
    {
        ResolveSound(SoftSound);
    }
}

void UDialogAudioComponent::PlayHintDialogue(FName HintID)
{
    if (HintID.IsNone())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    UGameJamGameInstance* GameJamGameInstance = Cast<UGameJamGameInstance>(GameInstance);
    if (!GameJamGameInstance)
    {
        UE_LOG(LogDialogAudio, Warning, TEXT("DialogAudioComponent could not access GameJamGameInstance."));
        return;
    }

    const TArray<FHintData> AllHints = GameJamGameInstance->GetAllHints();
    for (const FHintData& Hint : AllHints)
    {
        if (Hint.HintID == HintID)
        {
            if (Hint.DialogAudio.Num() == 0)
            {
                UE_LOG(LogDialogAudio, Warning, TEXT("Hint '%s' does not define any dialog audio."), *HintID.ToString());
                return;
            }

            TArray<TSoftObjectPtr<USoundBase>> SoftLines;
            for (const FString& AssetPath : Hint.DialogAudio)
            {
                if (AssetPath.IsEmpty())
                {
                    continue;
                }

                SoftLines.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(AssetPath)));
            }

            if (SoftLines.Num() == 0)
            {
                UE_LOG(LogDialogAudio, Warning, TEXT("Hint '%s' dialog audio entries failed to convert to soft references."), *HintID.ToString());
                return;
            }

            QueueDialogue(SoftLines, QueueGapSeconds);
            return;
        }
    }

    UE_LOG(LogDialogAudio, Warning, TEXT("Hint '%s' could not be found when requesting dialog."), *HintID.ToString());
}

bool UDialogAudioComponent::PlayIfNotCoolingDown(FName Tag, float CooldownSeconds)
{
    if (!GetWorld())
    {
        return false;
    }

    float& LastPlay = CooldownTimestamps.FindOrAdd(Tag);
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastPlay < CooldownSeconds)
    {
        return false;
    }

    LastPlay = CurrentTime;
    return true;
}

void UDialogAudioComponent::ApplyAudioSettings()
{
    if (!AudioComponent)
    {
        return;
    }

    AudioComponent->bAutoDestroy = bAutoDestroyOnFinish;
    AudioComponent->Priority = Priority;
    AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
    AudioComponent->SetPitchMultiplier(PitchMultiplier);
    AudioComponent->bUseAttachParentBound = true;
    AudioComponent->bAllowSpatialization = true;

    if (SoundClassOverride)
    {
        AudioComponent->SoundClassOverride = SoundClassOverride;
    }

    AudioComponent->ConcurrencySet.Empty();
    if (ConcurrencySettings)
    {
        AudioComponent->ConcurrencySet.Add(ConcurrencySettings);
    }

    FSoundAttenuationSettings EffectiveAttenuation;

    if (AttenuationSettings)
    {
        EffectiveAttenuation = AttenuationSettings->Attenuation;
    }
    else
    {
        FallbackAttenuation.FalloffDistance = DefaultMaxDistance;
        EffectiveAttenuation = FallbackAttenuation;
    }

    EffectiveAttenuation.bEnableOcclusion = bEnableOcclusion;
    EffectiveAttenuation.OcclusionTraceChannel = ECC_Visibility;

    AudioComponent->AttenuationSettings = AttenuationSettings;
    AudioComponent->AttenuationOverrides = EffectiveAttenuation;
    AudioComponent->bOverrideAttenuation = true;
}

void UDialogAudioComponent::InternalPlaySound(USoundBase* Sound, float FadeInTime, float StartTime, bool bIsDialogue, float SubtitleDurationOverride, const FText& Subtitle, bool bFromQueue)
{
    if (!AudioComponent || !Sound)
    {
        if (!Sound)
        {
            UE_LOG(LogDialogAudio, Warning, TEXT("DialogAudioComponent attempted to play an invalid sound."));
        }
        return;
    }

    if (ShouldCullPlayback(Sound))
    {
        UE_LOG(LogDialogAudio, Verbose, TEXT("DialogAudioComponent culled playback for '%s'."), *Sound->GetName());
        return;
    }

    if (!bFromQueue)
    {
        ClearQueue();
    }

    ActiveSound = Sound;
    AudioComponent->SetSound(Sound);
    AudioComponent->bIsUISound = false;
    AudioComponent->bAutoDestroy = bAutoDestroyOnFinish;
    if (!bFromQueue)
    {
        bPendingLoopingState = false;
    }

    const bool bShouldLoop = bFromQueue ? bPendingLoopingState : false;
    AudioComponent->SetLooping(bShouldLoop);

    ApplyAudioSettings();

    if (FadeInTime > 0.f)
    {
        AudioComponent->FadeIn(FadeInTime, 1.0f, StartTime);
    }
    else
    {
        AudioComponent->Play(StartTime);
    }

    OnLineStarted.Broadcast(Sound);

    if (bIsDialogue)
    {
        UpdateDucking(true);
        if (bEnableSubtitles)
        {
            CurrentSubtitle = Subtitle;
            OnSubtitleChanged.Broadcast(CurrentSubtitle);

            if (UWorld* World = GetWorld())
            {
                const float Duration = SubtitleDurationOverride > 0.f ? SubtitleDurationOverride : Sound->GetDuration();
                const float ClampedDuration = FMath::Max(Duration, 0.1f);
                World->GetTimerManager().ClearTimer(SubtitleTimerHandle);
                World->GetTimerManager().SetTimer(SubtitleTimerHandle, this, &UDialogAudioComponent::ClearSubtitle, ClampedDuration, false);
            }
        }
    }
    else if (AudioComponent->IsPlaying())
    {
        UpdateDucking(false);
    }
}

bool UDialogAudioComponent::ShouldCullPlayback(const USoundBase* Sound) const
{
    if (!bCullingEnabled || !GetWorld() || !GetOwner())
    {
        return false;
    }

    const AActor* OwnerActor = GetOwner();
    FVector ListenerLocation = FVector::ZeroVector;
    bool bHasListener = false;

    if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
    {
        ListenerLocation = CameraManager->GetCameraLocation();
        bHasListener = true;
    }
    else if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        ListenerLocation = Pawn->GetActorLocation();
        bHasListener = true;
    }

    if (!bHasListener)
    {
        return false;
    }

    const FVector OwnerLocation = OwnerActor->GetActorLocation();
    const float MaxDistance = GetEffectiveMaxDistance();
    const float CullDistance = MaxDistance * CullingDistanceMultiplier;

    if (CullDistance <= 0.f)
    {
        return false;
    }

    const float DistanceSquared = FVector::DistSquared(OwnerLocation, ListenerLocation);
    if (DistanceSquared > FMath::Square(CullDistance))
    {
        UE_LOG(LogDialogAudio, Verbose, TEXT("Skipping playback for '%s' due to distance culling."), Sound ? *Sound->GetName() : TEXT("None"));
        return true;
    }

    return false;
}

float UDialogAudioComponent::GetEffectiveMaxDistance() const
{
    if (AttenuationSettings)
    {
        const float ShapeRadius = AttenuationSettings->Attenuation.AttenuationShapeExtents.X;
        return FMath::Max(AttenuationSettings->Attenuation.FalloffDistance + ShapeRadius, 1.f);
    }

    return FMath::Max(DefaultMaxDistance, 1.f);
}

void UDialogAudioComponent::PlayNextQueuedLine()
{
    if (!bQueueActive)
    {
        return;
    }

    if (!GetWorld())
    {
        ClearQueue();
        return;
    }

    if (!QueuedLines.IsValidIndex(QueueIndex))
    {
        bQueueActive = false;
        OnQueueFinished.Broadcast();
        UpdateDucking(false);
        return;
    }

    USoundBase* Sound = ResolveSound(QueuedLines[QueueIndex]);
    ++QueueIndex;

    if (!Sound)
    {
        if (QueuedLines.IsValidIndex(QueueIndex))
        {
            if (UWorld* World = GetWorld())
            {
                if (QueueGapSeconds <= KINDA_SMALL_NUMBER)
                {
                    HandleQueueGapElapsed();
                }
                else
                {
                    World->GetTimerManager().SetTimer(QueueGapTimerHandle, this, &UDialogAudioComponent::HandleQueueGapElapsed, QueueGapSeconds, false);
                }
            }
        }
        else
        {
            bQueueActive = false;
            OnQueueFinished.Broadcast();
            UpdateDucking(false);
        }
        return;
    }

    const bool bIsLastLine = (QueueIndex >= QueuedLines.Num());
    bPendingLoopingState = bLoopLast && bIsLastLine;

    InternalPlaySound(Sound, 0.05f, 0.f, true, -1.f, FText(), true);

    if (bIsLastLine && bLoopLast)
    {
        bQueueActive = false;
        OnQueueFinished.Broadcast();
    }
}

void UDialogAudioComponent::HandleQueueGapElapsed()
{
    PlayNextQueuedLine();
}

void UDialogAudioComponent::ClearQueue()
{
    bQueueActive = false;
    QueuedLines.Reset();
    QueueIndex = 0;
    bPendingLoopingState = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(QueueGapTimerHandle);
    }
}

void UDialogAudioComponent::ClearSubtitle()
{
    if (!bEnableSubtitles)
    {
        CurrentSubtitle = FText::GetEmpty();
        return;
    }

    if (!CurrentSubtitle.IsEmpty())
    {
        CurrentSubtitle = FText::GetEmpty();
        OnSubtitleChanged.Broadcast(CurrentSubtitle);
    }
}

void UDialogAudioComponent::UpdateDucking(bool bShouldDuck)
{
    if (bShouldDuck == bDuckingActive)
    {
        return;
    }

    bDuckingActive = bShouldDuck;
    OnDuckingChanged.Broadcast(bDuckingActive, DuckingAmount);
}

void UDialogAudioComponent::UpdateAttachment()
{
    if (!AudioComponent)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    USceneComponent* AttachComponent = OwnerActor->GetRootComponent();

    if (AttachSocketName != NAME_None)
    {
        if (USkeletalMeshComponent* SkeletalMesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (SkeletalMesh->DoesSocketExist(AttachSocketName))
            {
                AudioComponent->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::KeepRelativeTransform, AttachSocketName);
                return;
            }

            UE_LOG(LogDialogAudio, Warning, TEXT("Socket '%s' was not found on mesh '%s', falling back to root."), *AttachSocketName.ToString(), *SkeletalMesh->GetName());
        }
    }

    if (AttachComponent)
    {
        AudioComponent->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepRelativeTransform);
    }
}

void UDialogAudioComponent::HandleAudioFinished()
{
    USoundBase* FinishedSound = ActiveSound.Get();
    ActiveSound.Reset();

    OnLineFinished.Broadcast(FinishedSound);

    if (bQueueActive)
    {
        if (UWorld* World = GetWorld())
        {
            if (QueueGapSeconds <= KINDA_SMALL_NUMBER)
            {
                HandleQueueGapElapsed();
            }
            else
            {
                World->GetTimerManager().SetTimer(QueueGapTimerHandle, this, &UDialogAudioComponent::HandleQueueGapElapsed, QueueGapSeconds, false);
            }
        }
    }
    else
    {
        UpdateDucking(false);
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(SubtitleTimerHandle);
            ClearSubtitle();
        }
    }
}

USoundBase* UDialogAudioComponent::ResolveSound(TSoftObjectPtr<USoundBase> Sound)
{
    if (Sound.IsValid())
    {
        return Sound.Get();
    }

    const FSoftObjectPath AssetPath = Sound.ToSoftObjectPath();
    if (!AssetPath.IsValid())
    {
        OnLoadFailed.Broadcast(AssetPath);
        UE_LOG(LogDialogAudio, Warning, TEXT("Soft dialog sound reference is invalid."));
        return nullptr;
    }

    USoundBase* LoadedSound = Sound.LoadSynchronous();
    if (!LoadedSound)
    {
        OnLoadFailed.Broadcast(AssetPath);
        UE_LOG(LogDialogAudio, Warning, TEXT("Failed to load dialog sound '%s'."), *AssetPath.ToString());
        return nullptr;
    }

    return LoadedSound;
}

