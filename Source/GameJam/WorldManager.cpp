#include "WorldManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

TWeakObjectPtr<AWorldManager> AWorldManager::ActiveWorldManager = nullptr;

AWorldManager::AWorldManager()
{
    PrimaryActorTick.bCanEverTick = false;

    StartingWorld = EWorldState::Light;
    CurrentWorld = StartingWorld;
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
    CurrentWorld = StartingWorld;

    InitializeInput();
    BroadcastWorldShift();
}

void AWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ActiveWorldManager.Get() == this)
    {
        ActiveWorldManager = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AWorldManager::InitializeInput()
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        EnableInput(PC);

        if (InputComponent)
        {
            InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AWorldManager::ShiftToNextWorld);
            InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AWorldManager::ShiftToPreviousWorld);
        }
    }
}

void AWorldManager::SetWorld(EWorldState NewWorld)
{
    if (CurrentWorld == NewWorld)
    {
        return;
    }

    CurrentWorld = NewWorld;
    BroadcastWorldShift();
}

void AWorldManager::ShiftToNextWorld()
{
    SetWorld(GetNextWorld(CurrentWorld));
}

void AWorldManager::ShiftToPreviousWorld()
{
    SetWorld(GetPreviousWorld(CurrentWorld));
}

void AWorldManager::BroadcastWorldShift()
{
    OnWorldShifted.Broadcast(CurrentWorld);
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
