#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDied);

UCLASS(ClassGroup=(Game), meta=(BlueprintSpawnableComponent))
class GAMEJAM_API UHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHealthComponent();

    /** Maximum health. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
    float MaxHealth = 100.f;

    /** Current health (clamped to [0, MaxHealth]). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
    float CurrentHealth = 100.f;

    /** Broadcast on any health change (after clamping). */
    UPROPERTY(BlueprintAssignable, Category="Health")
    FOnHealthChanged OnHealthChanged;

    /** Broadcast once when health reaches zero. */
    UPROPERTY(BlueprintAssignable, Category="Health")
    FOnDied OnDied;

    /** Add a signed delta to health (negative = damage). Returns true if changed. */
    UFUNCTION(BlueprintCallable, Category="Health")
    bool ApplyHealthDelta(float Delta);

    /** Explicit damage (positive value). Returns true if changed. */
    UFUNCTION(BlueprintCallable, Category="Health")
    bool ApplyDamage(float Damage);

    /** Explicit heal (positive value). Returns true if changed. */
    UFUNCTION(BlueprintCallable, Category="Health")
    bool Heal(float Amount);

    /** Set absolute health value (clamped). Returns true if changed. */
    UFUNCTION(BlueprintCallable, Category="Health")
    bool SetHealth(float NewHealth);

    /** Getters */
    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category="Health")
    bool IsAlive() const { return CurrentHealth > 0.f; }

protected:
    virtual void BeginPlay() override;

private:
    void BroadcastIfChanged(float Old, float OldMax);
    void ClampToValidRange();
};
