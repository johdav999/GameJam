#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameJamGameInstance.generated.h"

class UGameJamSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoopCountChanged, int32, NewLoopCount);

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

    /** Broadcast whenever the loop count changes. */
    UPROPERTY(BlueprintAssignable, Category = "Loop")
    FOnLoopCountChanged OnLoopCountChanged;

protected:
    /** Loads the loop data from disk or creates a new save when none exists. */
    void LoadLoopData();

    /** Writes the current loop data to disk. */
    void SaveLoopData();

    /** Helper used to apply the supplied loop count and notify listeners. */
    void ApplyLoopCount(int32 NewLoopCount, bool bFromLoad = false);

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
};
