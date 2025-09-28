#include "WorldManager.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "WorldShiftComponent.h"

namespace
{
    constexpr int32 GetWorldStateCount()
    {
        return static_cast<int32>(EWorldState::Dream) + 1;
    }

    int32 WrapWorldIndex(int32 Index)
    {
        const int32 WorldCount = GetWorldStateCount();
        Index %= WorldCount;
        if (Index < 0)
        {
            Index += WorldCount;
        }
        return Index;
    }
}

AWorldManager::AWorldManager()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentWorld = EWorldState::Light;
}

void AWorldManager::BeginPlay()
{
    Super::BeginPlay();

    BroadcastWorldShift(CurrentWorld);
}

void AWorldManager::SetWorld(EWorldState NewWorld)
{
    if (CurrentWorld == NewWorld)
    {
        return;
    }

    CurrentWorld = NewWorld;
    BroadcastWorldShift(CurrentWorld);
}

void AWorldManager::CycleWorld(int32 Direction)
{
    if (Direction == 0)
    {
        return;
    }

    Direction = FMath::Clamp(Direction, -1, 1);
    const int32 WorldCount = GetWorldStateCount();
    const int32 CurrentIndex = static_cast<int32>(CurrentWorld);
    const int32 NextIndex = WrapWorldIndex(CurrentIndex + Direction);

    SetWorld(static_cast<EWorldState>(NextIndex));
}

void AWorldManager::BroadcastWorldShift(EWorldState NewWorld)
{
    OnWorldShifted.Broadcast(NewWorld);

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        TArray<UWorldShiftComponent*> Components;
        Actor->GetComponents<UWorldShiftComponent>(Components);
        for (UWorldShiftComponent* Component : Components)
        {
            if (Component)
            {
                Component->HandleWorldShift(NewWorld);
            }
        }
    }
}

AWorldManager* AWorldManager::GetWorldManager(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
    {
        for (TActorIterator<AWorldManager> It(World); It; ++It)
        {
            return *It;
        }
    }

    return nullptr;
}
