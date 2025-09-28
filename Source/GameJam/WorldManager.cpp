#include "WorldManager.h"

#include "AudioDevice.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundMix.h"

TWeakObjectPtr<AWorldManager> AWorldManager::ActiveWorldManager = nullptr;

AWorldManager::AWorldManager()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
    PostProcessComponent->SetupAttachment(RootComponent);
    PostProcessComponent->bUnbound = true;
    PostProcessComponent->BlendWeight = 1.0f;

    StartingWorld = EWorldState::Light;
    CurrentWorld = StartingWorld;
    SoundMixFadeTime = 0.5f;
}

AWorldManager* AWorldManager::Get(UWorld* World)
{
    if (ActiveWorldManager.IsValid())
    {
        return ActiveWorldManager.Get();
    }

    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AWorldManager> It(World); It; ++It)
    {
        if (AWorldManager* Manager = *It)
        {
            ActiveWorldManager = Manager;
            return Manager;
        }
    }

    return nullptr;
}

void AWorldManager::BeginPlay()
{
    Super::BeginPlay();

    ActiveWorldManager = this;

    if (PostProcessComponent)
    {
        DefaultPostProcessSettings = PostProcessComponent->Settings;
    }

    CurrentWorld = StartingWorld;

    ApplyWorldFeedback(CurrentWorld);
    OnWorldShifted.Broadcast(CurrentWorld);
}

void AWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ActiveWorldManager.Get() == this)
    {
        ActiveWorldManager = nullptr;
    }

    if (FAudioDevice* AudioDevice = GetWorld() ? GetWorld()->GetAudioDeviceRaw() : nullptr)
    {
        if (ActiveSoundMix.IsValid())
        {
            AudioDevice->PopSoundMixModifier(ActiveSoundMix.Get(), SoundMixFadeTime);
        }

        if (DefaultSoundMix)
        {
            AudioDevice->SetBaseSoundMix(DefaultSoundMix.Get());
        }
    }

    ActiveSoundMix.Reset();

    Super::EndPlay(EndPlayReason);
}

void AWorldManager::SetWorld(EWorldState NewWorld)
{
    if (CurrentWorld == NewWorld)
    {
		UE_LOG(LogTemp, Warning, TEXT("Already in world: %d"), (int32)CurrentWorld);
        return;
    }
	UE_LOG(LogTemp, Warning, TEXT("Shifting world from %d to %d"), (int32)CurrentWorld, (int32)NewWorld);
    CurrentWorld = NewWorld;

    ApplyWorldFeedback(CurrentWorld);
    OnWorldShifted.Broadcast(CurrentWorld);
}

void AWorldManager::CycleWorld(int32 Direction)
{
    if (Direction == 0)
    {
        return;
    }

    const int32 Steps = FMath::Abs(Direction);
    EWorldState TargetWorld = CurrentWorld;
    for (int32 StepIndex = 0; StepIndex < Steps; ++StepIndex)
    {
        TargetWorld = (Direction > 0) ? GetNextWorld(TargetWorld) : GetPreviousWorld(TargetWorld);
    }

    SetWorld(TargetWorld);
}

void AWorldManager::ShiftToNextWorld()
{
    CycleWorld(1);
}

void AWorldManager::ShiftToPreviousWorld()
{
    CycleWorld(-1);
}

void AWorldManager::ApplyWorldFeedback(EWorldState NewWorld)
{
    ApplyPostProcessForWorld(NewWorld);
    ApplyAudioForWorld(NewWorld);
}

void AWorldManager::ApplyPostProcessForWorld(EWorldState NewWorld)
{
    if (!PostProcessComponent)
    {
        return;
    }

    if (const FPostProcessSettings* Settings = WorldPostProcessSettings.Find(NewWorld))
    {
        PostProcessComponent->Settings = *Settings;
    }
    else
    {
        PostProcessComponent->Settings = DefaultPostProcessSettings;
    }

    PostProcessComponent->BlendWeight = 1.0f;
}

void AWorldManager::ApplyAudioForWorld(EWorldState NewWorld)
{
    USoundMix* DesiredMix = nullptr;
    if (TObjectPtr<USoundMix>* const MixPtr = WorldSoundMixes.Find(NewWorld))
    {
        DesiredMix = MixPtr->Get();
    }

    if (!DesiredMix)
    {
        DesiredMix = DefaultSoundMix.Get();
    }

    if (DesiredMix == ActiveSoundMix.Get())
    {
        return;
    }

    if (FAudioDevice* AudioDevice = GetWorld() ? GetWorld()->GetAudioDeviceRaw() : nullptr)
    {
        if (ActiveSoundMix.IsValid())
        {
            AudioDevice->PopSoundMixModifier(ActiveSoundMix.Get(), SoundMixFadeTime);
        }
	UE_LOG(LogTemp, Warning, TEXT("Shifting to previous world from: %d"), (int32)CurrentWorld);
    SetWorld(GetPreviousWorld(CurrentWorld));
}

        if (DesiredMix)
        {
            AudioDevice->PushSoundMixModifier(DesiredMix, true, true, SoundMixFadeTime);
            AudioDevice->SetBaseSoundMix(DesiredMix);
            ActiveSoundMix = DesiredMix;
        }
        else
        {
            ActiveSoundMix.Reset();
        }
    }
}

EWorldState AWorldManager::GetNextWorld(EWorldState InWorld)
{
    switch (InWorld)
    {
    case EWorldState::Light:
        return EWorldState::Shadow;
    case EWorldState::Shadow:
        return EWorldState::Chaos;
    case EWorldState::Chaos:
    default:
        return EWorldState::Light;
    }
}

EWorldState AWorldManager::GetPreviousWorld(EWorldState InWorld)
{
    switch (InWorld)
    {
    case EWorldState::Light:
        return EWorldState::Chaos;
    case EWorldState::Shadow:
        return EWorldState::Light;
    case EWorldState::Chaos:
    default:
        return EWorldState::Shadow;
    }
}
