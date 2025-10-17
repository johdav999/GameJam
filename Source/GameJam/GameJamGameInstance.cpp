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

    if (ActiveSave)
    {
        RestorePersistentHints(ActiveSave->PersistentHints);
    }

    const int32 LoadedLoopCount = ActiveSave ? ActiveSave->LoopCount : 0;
    ApplyLoopCount(LoadedLoopCount, true);

    OnHintCollectionChanged.Broadcast();
}

void UGameJamGameInstance::SaveLoopData()
{
    if (!ActiveSave)
    {
        return;
    }

    ActiveSave->LoopCount = LoopCount;
    ActiveSave->PersistentHints = GatherPersistentHints();
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

    CheckFutureHints(LoopCount);

    if (!bFromLoad)
    {
        SaveLoopData();
    }

    OnLoopCountChanged.Broadcast(LoopCount);
}

bool UGameJamGameInstance::AddHint(FName HintID, const FText& HintText, bool bIsPersistent, EHintTemporalState TemporalState, int32 LoopToUnlock)
{
    if (HintID.IsNone() || KnownHints.Contains(HintID))
    {
        return false;
    }

    FHintData NewHint;
    NewHint.HintID = HintID;
    NewHint.HintText = HintText;
    NewHint.bIsPersistent = bIsPersistent;
    NewHint.TemporalState = TemporalState;
    NewHint.LoopToUnlock = FMath::Max(0, LoopToUnlock);

    if (NewHint.TemporalState == EHintTemporalState::Future && NewHint.LoopToUnlock <= LoopCount)
    {
        NewHint.TemporalState = EHintTemporalState::Present;
        NewHint.LoopToUnlock = LoopCount;
    }
    else if (NewHint.TemporalState != EHintTemporalState::Future)
    {
        NewHint.LoopToUnlock = 0;
    }

    KnownHints.Add(HintID, NewHint);

    OnHintChanged.Broadcast(NewHint);
    OnHintCollectionChanged.Broadcast();

    if (NewHint.bIsPersistent)
    {
        SaveLoopData();
    }

    return true;
}

bool UGameJamGameInstance::HasHint(FName HintID) const
{
    return KnownHints.Contains(HintID);
}

bool UGameJamGameInstance::RevealHint(FName HintID)
{
    if (FHintData* Hint = KnownHints.Find(HintID))
    {
        if (Hint->TemporalState == EHintTemporalState::Future)
        {
            Hint->TemporalState = EHintTemporalState::Present;
            Hint->LoopToUnlock = LoopCount;

            OnHintChanged.Broadcast(*Hint);
            OnHintCollectionChanged.Broadcast();

            if (Hint->bIsPersistent)
            {
                SaveLoopData();
            }

            return true;
        }
    }

    return false;
}

TArray<FHintData> UGameJamGameInstance::GetVisibleHints() const
{
    TArray<FHintData> Result;
    Result.Reserve(KnownHints.Num());

    for (const TPair<FName, FHintData>& Pair : KnownHints)
    {
        if (Pair.Value.TemporalState == EHintTemporalState::Past || Pair.Value.TemporalState == EHintTemporalState::Present)
        {
            Result.Add(Pair.Value);
        }
    }

    return Result;
}

TArray<FHintData> UGameJamGameInstance::GetAllHints() const
{
    TArray<FHintData> Result;
    KnownHints.GenerateValueArray(Result);
    return Result;
}

void UGameJamGameInstance::ClearHintsOnReset()
{
    bool bCollectionChanged = false;
    bool bRequiresSave = false;

    TArray<FName> HintsToRemove;
    TArray<FHintData> UpdatedHints;

    for (TPair<FName, FHintData>& Pair : KnownHints)
    {
        FHintData& Hint = Pair.Value;

        if (!Hint.bIsPersistent)
        {
            HintsToRemove.Add(Pair.Key);
            bCollectionChanged = true;
            continue;
        }

        if (Hint.TemporalState == EHintTemporalState::Present)
        {
            Hint.TemporalState = EHintTemporalState::Past;
            UpdatedHints.Add(Hint);
            bCollectionChanged = true;
            bRequiresSave = true;
        }
    }

    for (const FName& HintID : HintsToRemove)
    {
        KnownHints.Remove(HintID);
    }

    for (const FHintData& Hint : UpdatedHints)
    {
        OnHintChanged.Broadcast(Hint);
    }

    if (bCollectionChanged)
    {
        OnHintCollectionChanged.Broadcast();
    }

    if (bRequiresSave)
    {
        SaveLoopData();
    }
}

void UGameJamGameInstance::ClearAllHints()
{
    if (KnownHints.Num() == 0)
    {
        // Even when nothing changes we still need to notify listeners so they can refresh.
        OnHintCollectionChanged.Broadcast();
        bDirty = true;
        SaveLoopData();
        return;
    }

    KnownHints.Empty();

    OnHintCollectionChanged.Broadcast();

    bDirty = true;
    SaveLoopData();
}

void UGameJamGameInstance::CheckFutureHints(int32 CurrentLoop)
{
    bool bCollectionChanged = false;
    bool bRequiresSave = false;

    TArray<FHintData> UpdatedHints;

    for (TPair<FName, FHintData>& Pair : KnownHints)
    {
        FHintData& Hint = Pair.Value;
        if (Hint.TemporalState == EHintTemporalState::Future && Hint.LoopToUnlock <= CurrentLoop)
        {
            Hint.TemporalState = EHintTemporalState::Present;
            Hint.LoopToUnlock = CurrentLoop;
            UpdatedHints.Add(Hint);
            bCollectionChanged = true;
            bRequiresSave |= Hint.bIsPersistent;
        }
    }

    for (const FHintData& Hint : UpdatedHints)
    {
        OnHintChanged.Broadcast(Hint);
    }

    if (bCollectionChanged)
    {
        OnHintCollectionChanged.Broadcast();
    }

    if (bRequiresSave)
    {
        SaveLoopData();
    }
}

void UGameJamGameInstance::HandleWorldReset()
{
    ClearHintsOnReset();
}

TArray<FHintData> UGameJamGameInstance::GatherPersistentHints() const
{
    TArray<FHintData> Result;
    for (const TPair<FName, FHintData>& Pair : KnownHints)
    {
        if (Pair.Value.bIsPersistent)
        {
            Result.Add(Pair.Value);
        }
    }

    return Result;
}

void UGameJamGameInstance::RestorePersistentHints(const TArray<FHintData>& SavedHints)
{
    KnownHints.Empty();

    for (const FHintData& Hint : SavedHints)
    {
        if (!Hint.HintID.IsNone())
        {
            KnownHints.Add(Hint.HintID, Hint);
        }
    }
}
