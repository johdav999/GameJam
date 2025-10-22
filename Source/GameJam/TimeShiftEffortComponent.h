#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeShiftEffortComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffortChanged, float, NewEffort);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEffortDepleted);

/**
 * Tracks the player's available effort for maintaining the Chaos world and
 * automatically drains or replenishes it based on the active world state.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEJAM_API UTimeShiftEffortComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimeShiftEffortComponent();

    /** Current amount of effort available for maintaining the Chaos world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TimeShift")
    float CurrentEffort = 100.0f;

    /** Maximum amount of effort that can be stored. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TimeShift")
    float MaxEffort = 100.0f;

    /** Effort drained per second while sustaining the Chaos world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TimeShift")
    float DepletionRate = 10.0f;

    /** Effort restored per second while the Light world is active. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TimeShift")
    float RecoveryRate = 10.0f;

    /** Broadcast whenever the current effort value changes. */
    UPROPERTY(BlueprintAssignable, Category="TimeShift")
    FOnEffortChanged OnEffortChanged;

    /** Broadcast when the effort meter becomes fully depleted. */
    UPROPERTY(BlueprintAssignable, Category="TimeShift")
    FOnEffortDepleted OnEffortDepleted;

    /** Applies a signed change to the current effort, clamped to [0, MaxEffort]. */
    UFUNCTION(BlueprintCallable, Category="TimeShift")
    bool ModifyEffort(float Delta);

    /** Sets the current effort to the provided value, clamped to [0, MaxEffort]. */
    UFUNCTION(BlueprintCallable, Category="TimeShift")
    bool SetEffort(float NewEffort);

    /** Returns the current effort. */
    UFUNCTION(BlueprintPure, Category="TimeShift")
    float GetCurrentEffort() const { return CurrentEffort; }

    /** Returns the maximum effort. */
    UFUNCTION(BlueprintPure, Category="TimeShift")
    float GetMaxEffort() const { return MaxEffort; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void ClampEffort();
    void BroadcastIfChanged(float PreviousEffort);
    bool ShouldDepleteEffort() const;
};
