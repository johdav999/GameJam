#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldShiftTypes.h"
#include "WorldShiftEffectsComponent.generated.h"

class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldShiftTriggered, EWorldState, NewWorld);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorldShiftHealthDrained, float, NewHealth, float, MaxHealth);

/**
 * Component responsible for orchestrating audiovisual feedback and gameplay side-effects when the player
 * transitions between world states.
 */
UCLASS(ClassGroup = (WorldShift), meta = (BlueprintSpawnableComponent))
class GAMEJAM_API UWorldShiftEffectsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWorldShiftEffectsComponent();

    /** Health cost applied every time the player performs a world shift. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Cost")
    float HealthCostPerSwitch;

    /** Audio cues to play when shifting into a specific world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Audio")
    TMap<EWorldState, USoundBase*> SwitchSounds;

    /** Niagara particle systems to spawn when shifting into a specific world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|VFX")
    TMap<EWorldState, UNiagaraSystem*> SwitchParticles;

    /** Color tint applied to the temporary post process flash when entering a world. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Visual")
    TMap<EWorldState, FLinearColor> SwitchColors;

    /** Amount of time it takes for the post process flash to fade out. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Shift|Visual")
    float PostProcessBlendDuration;

    /** Triggers all configured effects for the provided world state. */
    UFUNCTION(BlueprintCallable, Category = "World Shift|Effects")
    void TriggerWorldShiftEffects(EWorldState NewWorld);

protected:
    /** Starts a short-lived post process flash that fades away over time. */
    UFUNCTION(BlueprintCallable, Category = "World Shift|Effects")
    void StartPostProcessFlash(FLinearColor FlashColor);

    /** Allows designers to hook in bespoke effects (camera shake, slow motion, etc.) for specific worlds. */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Shift|Effects")
    void OnSpecialWorldEffect(EWorldState NewWorld);

public:
    /** Event fired whenever a world shift effect is triggered. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift|Events")
    FOnWorldShiftTriggered OnWorldShiftTriggered;

    /** Event fired after health has been drained by a world shift. */
    UPROPERTY(BlueprintAssignable, Category = "World Shift|Events")
    FOnWorldShiftHealthDrained OnHealthDrained;

private:
    bool ApplyHealthCost(float& OutNewHealth, float& OutMaxHealth);
    UActorComponent* ResolveHealthComponent() const;
    bool TryInvokeHealthDelta(UActorComponent* HealthComponent, float Delta) const;
    bool TryGetHealthValues(UActorComponent* HealthComponent, float& OutCurrentHealth, float& OutMaxHealth) const;
    bool TryGetHealthValueFromFunction(UActorComponent* HealthComponent, const TArray<FName>& FunctionNames, float& OutValue) const;
    bool TryGetHealthValueFromProperty(UActorComponent* HealthComponent, const TArray<FName>& PropertyNames, float& OutValue) const;
};
