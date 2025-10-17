#include "GameJamGameInstance.h"

#include "GameJamSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UGameJamGameInstance::LoopSaveSlot(TEXT("ThreeWorldsLoopData"));
const int32 UGameJamGameInstance::LoopSaveUserIndex = 0;

UGameJamGameInstance::UGameJamGameInstance()
    : LoopCount(0)
    , bDirty(false)
{
}

void UGameJamGameInstance::Init()
{
    Super::Init();

    LoadLoopData();
}

void UGameJamGameInstance::Shutdown()
{
    if (bDirty)
    {
        SaveLoopData();
    }

    Super::Shutdown();
}

void UGameJamGameInstance::SetLoopCount(int32 NewLoopCount)
{
    ApplyLoopCount(FMath::Max(0, NewLoopCount));
}

void UGameJamGameInstance::IncrementLoopCount()
{
    ApplyLoopCount(LoopCount + 1);
}

void UGameJamGameInstance::ResetLoopCount()
{
    ApplyLoopCount(0);
}

void UGameJamGameInstance::LoadLoopData()
{
    ActiveSave = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(LoopSaveSlot, LoopSaveUserIndex))
    {
        if (USaveGame* LoadedSave = UGameplayStatics::LoadGameFromSlot(LoopSaveSlot, LoopSaveUserIndex))
        {
            ActiveSave = Cast<UGameJamSaveGame>(LoadedSave);
        }
    }

    if (!ActiveSave)
    {
        ActiveSave = Cast<UGameJamSaveGame>(UGameplayStatics::CreateSaveGameObject(UGameJamSaveGame::StaticClass()));
    }

    const int32 LoadedLoopCount = ActiveSave ? ActiveSave->LoopCount : 0;
    ApplyLoopCount(LoadedLoopCount, true);
}

void UGameJamGameInstance::SaveLoopData()
{
    if (!ActiveSave)
    {
        return;
    }

    ActiveSave->LoopCount = LoopCount;
    UGameplayStatics::SaveGameToSlot(ActiveSave, LoopSaveSlot, LoopSaveUserIndex);
    bDirty = false;
}

void UGameJamGameInstance::ApplyLoopCount(int32 NewLoopCount, bool bFromLoad)
{
    if (LoopCount == NewLoopCount)
    {
        if (bFromLoad)
        {
            OnLoopCountChanged.Broadcast(LoopCount);
        }
        return;
    }

    LoopCount = NewLoopCount;
    bDirty = !bFromLoad;

    if (!bFromLoad)
    {
        SaveLoopData();
    }

    OnLoopCountChanged.Broadcast(LoopCount);
}
