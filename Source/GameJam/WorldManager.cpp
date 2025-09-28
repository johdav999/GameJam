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
		UE_LOG(LogTemp, Warning, TEXT("Already in world: %d"), (int32)CurrentWorld);
        return;
    }
	UE_LOG(LogTemp, Warning, TEXT("Shifting world from %d to %d"), (int32)CurrentWorld, (int32)NewWorld);
    CurrentWorld = NewWorld;
    BroadcastWorldShift();
}

void AWorldManager::ShiftToNextWorld()
{
   EWorldState worldState= GetNextWorld(CurrentWorld);
    SetWorld(worldState);
	UE_LOG(LogTemp, Warning, TEXT("Shifted to world: %d"), (int32)CurrentWorld);
}

void AWorldManager::ShiftToPreviousWorld()
{
	UE_LOG(LogTemp, Warning, TEXT("Shifting to previous world from: %d"), (int32)CurrentWorld);
    SetWorld(GetPreviousWorld(CurrentWorld));
}

void AWorldManager::BroadcastWorldShift()
{
	UE_LOG(LogTemp, Warning, TEXT("Broadcasting world shift to: %d"), (int32)CurrentWorld);
    OnWorldShifted.Broadcast(CurrentWorld);
}

EWorldState AWorldManager::GetNextWorld(EWorldState InWorld)
{
    switch (InWorld)
    {
    case EWorldState::Light:
        return EWorldState::Shadow;
    case EWorldState::Shadow:
        return EWorldState::Dream;
    case EWorldState::Dream:
    default:
        return EWorldState::Light;
    }
}

EWorldState AWorldManager::GetPreviousWorld(EWorldState InWorld)
{
    switch (InWorld)
    {
    case EWorldState::Light:
        return EWorldState::Dream;
    case EWorldState::Shadow:
        return EWorldState::Light;
    case EWorldState::Dream:
    default:
        return EWorldState::Shadow;
    }
}
