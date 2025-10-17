#include "WorldManager.h"

#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameJamGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"

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
    MusicFadeTime = 0.5f;
    CycleInterval = 5.0f;
    PreWarningTime = 1.0f;
    bGlobalTimedSolid = true;
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

    OnTimedSolidPhaseChanged.Broadcast(bGlobalTimedSolid);
    StartGlobalTimedSolidCycle();
}

void AWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopGlobalTimedSolidCycle();

    if (ActiveWorldManager.Get() == this)
    {
        ActiveWorldManager = nullptr;
    }

    if (ActiveMusicComponent.IsValid())
    {
        if (MusicFadeTime > 0.0f)
        {
            ActiveMusicComponent->FadeOut(MusicFadeTime, 0.0f);
        }
        else
        {
            ActiveMusicComponent->Stop();
        }

        ActiveMusicComponent.Reset();
    }

    Super::EndPlay(EndPlayReason);
}

void AWorldManager::SetWorld(EWorldState NewWorld)
{
    if (CurrentWorld == NewWorld)
    {
        return;
    }
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

void AWorldManager::SetResetCheckpoint(AActor* NewCheckpoint)
{
    if (NewCheckpoint && NewCheckpoint->GetWorld() != GetWorld())
    {
        return;
    }

    ActiveResetCheckpoint = NewCheckpoint;
}

void AWorldManager::ResetWorld()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UGameJamGameInstance* GameInstance = Cast<UGameJamGameInstance>(World->GetGameInstance()))
    {
        GameInstance->IncrementLoopCount();
    }

    StopGlobalTimedSolidCycle();

    bGlobalTimedSolid = true;
    OnTimedSolidPhaseChanged.Broadcast(bGlobalTimedSolid);

    CurrentWorld = EWorldState::Light;
    ApplyWorldFeedback(CurrentWorld);
    OnWorldShifted.Broadcast(CurrentWorld);

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
    if (PlayerPawn)
    {
        AActor* SpawnActor = ActiveResetCheckpoint.IsValid() ? ActiveResetCheckpoint.Get() : nullptr;
        if (!SpawnActor && DefaultResetSpawnPoint)
        {
            SpawnActor = DefaultResetSpawnPoint.Get();
        }
        if (!SpawnActor)
        {
            SpawnActor = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass());
        }

        FTransform SpawnTransform = PlayerPawn->GetActorTransform();
        if (SpawnActor)
        {
            SpawnTransform = SpawnActor->GetActorTransform();
        }

        const FVector SpawnLocation = SpawnTransform.GetLocation();
        const FRotator SpawnRotation = SpawnTransform.GetRotation().Rotator();

        if (ACharacter* Character = Cast<ACharacter>(PlayerPawn))
        {
            Character->TeleportTo(SpawnLocation, SpawnRotation, false, true);

            if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
            {
                MovementComponent->StopMovementImmediately();
            }
        }
        else
        {
            PlayerPawn->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

            if (UMovementComponent* Movement = PlayerPawn->GetMovementComponent())
            {
                Movement->StopMovementImmediately();
            }
        }

        if (AController* Controller = PlayerPawn->GetController())
        {
            Controller->SetControlRotation(SpawnRotation);
        }
    }

    StartGlobalTimedSolidCycle();
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
    if (ActiveMusicComponent.IsValid())
    {
        if (MusicFadeTime > 0.0f)
        {
            ActiveMusicComponent->FadeOut(MusicFadeTime, 0.0f);
        }
        else
        {
            ActiveMusicComponent->Stop();
        }

        ActiveMusicComponent.Reset();
    }

    const TObjectPtr<USoundBase>* SongPtr = WorldSongs.Find(NewWorld);
    USoundBase* WorldSong = SongPtr ? SongPtr->Get() : nullptr;
    if (!WorldSong)
    {
        return;
    }

    UAudioComponent* NewMusicComponent = NewObject<UAudioComponent>(this);
    if (NewMusicComponent)
    {
        NewMusicComponent->bAutoActivate = false;
        NewMusicComponent->SetSound(WorldSong);
        NewMusicComponent->RegisterComponent(); // make it part of the world
    }

    if (!NewMusicComponent)
    {
        return;
    }

    if (const TObjectPtr<USoundSubmix>* SubmixPtr = WorldSubmixes.Find(NewWorld))
    {      
            if (USoundSubmix* Submix = SubmixPtr->Get())
            {
                // Route the new audio component fully into the chosen submix
                NewMusicComponent->SetSubmixSend(Submix, 1.0f);
            }        
    }

    ActiveMusicComponent = NewMusicComponent;

    if (MusicFadeTime > 0.0f)
    {
        NewMusicComponent->FadeIn(MusicFadeTime, 1.0f);
    }
    else
    {
        NewMusicComponent->SetVolumeMultiplier(1.0f);
    }
}

void AWorldManager::StartGlobalTimedSolidCycle()
{
    StopGlobalTimedSolidCycle();

    const float Interval = FMath::Max(0.01f, CycleInterval);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(GlobalTimedSolidHandle, this, &AWorldManager::HandleGlobalTimedSolidToggle, Interval, true, Interval);

        if (PreWarningTime > 0.0f && PreWarningTime < Interval)
        {
            const float Delay = Interval - PreWarningTime;
            World->GetTimerManager().SetTimer(PreWarningHandle, this, &AWorldManager::BroadcastPreWarning, Delay, false);
        }
    }
}

void AWorldManager::StopGlobalTimedSolidCycle()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GlobalTimedSolidHandle);
        World->GetTimerManager().ClearTimer(PreWarningHandle);
    }
}

void AWorldManager::HandleGlobalTimedSolidToggle()
{
    bGlobalTimedSolid = !bGlobalTimedSolid;
    OnTimedSolidPhaseChanged.Broadcast(bGlobalTimedSolid);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PreWarningHandle);

        const float Interval = FMath::Max(0.01f, CycleInterval);
        if (PreWarningTime > 0.0f && PreWarningTime < Interval)
        {
            const float Delay = Interval - PreWarningTime;
            World->GetTimerManager().SetTimer(PreWarningHandle, this, &AWorldManager::BroadcastPreWarning, Delay, false);
        }
    }
}

void AWorldManager::BroadcastPreWarning()
{
    OnTimedSolidPreWarning.Broadcast(!bGlobalTimedSolid);
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
