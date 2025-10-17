#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "HintTypes.h"
#include "GameJamGameInstance.generated.h"

class UGameJamSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoopCountChanged, int32, NewLoopCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHintChanged, const FHintData&, HintData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHintCollectionChanged);

/**
 * Game instance responsible for persisting gameplay state such as the loop counter.
 */
UCLASS()
class GAMEJAM_API UGameJamGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UGameJamGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    /** Returns the number of times the world has been reset. */
    UFUNCTION(BlueprintPure, Category = "Loop")
    int32 GetLoopCount() const { return LoopCount; }

    /** Sets the loop count to a specific value and persists the change. */
    UFUNCTION(BlueprintCallable, Category = "Loop")
    void SetLoopCount(int32 NewLoopCount);

    /** Increments the loop counter and persists the updated value. */
    UFUNCTION(BlueprintCallable, Category = "Loop")
    void IncrementLoopCount();

    /** Resets the loop counter back to zero and persists the change. */
    UFUNCTION(BlueprintCallable, Category = "Loop")
    void ResetLoopCount();

    /** Adds a new hint to the player's memory if it does not already exist. */
    UFUNCTION(BlueprintCallable, Category = "Hints")
    bool AddHint(FName HintID, const FText& HintText, bool bIsPersistent, EHintTemporalState TemporalState, int32 LoopToUnlock);

    /** Returns true if the player already knows the supplied hint. */
    UFUNCTION(BlueprintPure, Category = "Hints")
    bool HasHint(FName HintID) const;

    /** Reveals a future hint, transitioning it into the present timeline. */
    UFUNCTION(BlueprintCallable, Category = "Hints")
    bool RevealHint(FName HintID);

    /** Returns hints that should be shown prominently in the UI (past + present). */
    UFUNCTION(BlueprintPure, Category = "Hints")
    TArray<FHintData> GetVisibleHints() const;

    /** Returns every tracked hint regardless of its temporal state. */
    UFUNCTION(BlueprintPure, Category = "Hints")
    TArray<FHintData> GetAllHints() const;

    /** Clears temporary hints during a loop reset while preserving memories. */
    UFUNCTION(BlueprintCallable, Category = "Hints")
    void ClearHintsOnReset();

    /** Evaluates future hints and reveals them when their loop threshold is met. */
    UFUNCTION(BlueprintCallable, Category = "Hints")
    void CheckFutureHints(int32 CurrentLoop);

    /** Allows systems to notify the game instance that a reset has begun. */
    UFUNCTION(BlueprintCallable, Category = "Hints")
    void HandleWorldReset();

    /** Broadcast whenever the loop count changes. */
    UPROPERTY(BlueprintAssignable, Category = "Loop")
    FOnLoopCountChanged OnLoopCountChanged;

    /** Broadcast when a specific hint is added or changes state. */
    UPROPERTY(BlueprintAssignable, Category = "Hints")
    FOnHintChanged OnHintChanged;

    /** Broadcast when the set of available hints needs to be refreshed. */
    UPROPERTY(BlueprintAssignable, Category = "Hints")
    FOnHintCollectionChanged OnHintCollectionChanged;

protected:
    /** Loads the loop data from disk or creates a new save when none exists. */
    void LoadLoopData();

    /** Writes the current loop data to disk. */
    void SaveLoopData();

    /** Helper used to apply the supplied loop count and notify listeners. */
    void ApplyLoopCount(int32 NewLoopCount, bool bFromLoad = false);

    /** Filters out persistent hints so they can be saved between sessions. */
    TArray<FHintData> GatherPersistentHints() const;

    /** Restores persistent hints from a save slot. */
    void RestorePersistentHints(const TArray<FHintData>& SavedHints);

private:
    /** Slot name used to persist loop data between sessions. */
    static const FString LoopSaveSlot;

    /** User index used for saving the loop data. */
    static const int32 LoopSaveUserIndex;

    /** Cached save game instance that stores the loop count. */
    UPROPERTY()
    TObjectPtr<UGameJamSaveGame> ActiveSave;

    /** Number of times the world has been reset. */
    UPROPERTY(BlueprintReadOnly, Category = "Loop", meta = (AllowPrivateAccess = "true"))
    int32 LoopCount;

    /** Tracks whether the save data needs to be written to disk during shutdown. */
    bool bDirty;

    /** Collection of hints that the player has encountered. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hints", meta = (AllowPrivateAccess = "true"))
    TMap<FName, FHintData> KnownHints;
};
