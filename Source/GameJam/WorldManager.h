#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldShiftTypes.h"
#include "WorldManager.generated.h"

UCLASS(BlueprintType)
class GAMEJAM_API AWorldManager : public AActor
{
    GENERATED_BODY()

public:
    AWorldManager();

    static AWorldManager* Get(UWorld* World);

    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void SetWorld(EWorldState NewWorld);

    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void ShiftToNextWorld();

    UFUNCTION(BlueprintCallable, Category = "World Shift")
    void ShiftToPreviousWorld();

    UFUNCTION(BlueprintPure, Category = "World Shift")
    EWorldState GetCurrentWorld() const { return CurrentWorld; }

    UPROPERTY(BlueprintAssignable, Category = "World Shift")
    FOnWorldShifted OnWorldShifted;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializeInput();

private:
    static TWeakObjectPtr<AWorldManager> ActiveWorldManager;

    void BroadcastWorldShift();

    static EWorldState GetNextWorld(EWorldState InWorld);
    static EWorldState GetPreviousWorld(EWorldState InWorld);

    UPROPERTY(EditAnywhere, Category = "World Shift")
    EWorldState StartingWorld;

    EWorldState CurrentWorld;
};
