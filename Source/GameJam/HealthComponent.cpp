#include "HealthComponent.h"

#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
}

void UHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    ClampToValidRange();
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UHealthComponent::ClampToValidRange()
{
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
}

void UHealthComponent::BroadcastIfChanged(float Old, float OldMax)
{
    if (!FMath::IsNearlyEqual(Old, CurrentHealth) || !FMath::IsNearlyEqual(OldMax, MaxHealth))
    {
        OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
        if (CurrentHealth <= 0.f && Old > 0.f)
        {
            OnDied.Broadcast();
        }
    }
}

bool UHealthComponent::ApplyHealthDelta(float Delta)
{
    if (FMath::IsNearlyZero(Delta))
    {
        return false;
    }

    const float Old = CurrentHealth;
    const float OldMax = MaxHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + Delta, 0.f, MaxHealth);
    BroadcastIfChanged(Old, OldMax);
    return !FMath::IsNearlyEqual(Old, CurrentHealth);
}

bool UHealthComponent::ApplyDamage(float Damage)
{
    if (Damage <= 0.f)
    {
        return false;
    }

    return ApplyHealthDelta(-Damage);
}

bool UHealthComponent::Heal(float Amount)
{
    if (Amount <= 0.f)
    {
        return false;
    }

    return ApplyHealthDelta(Amount);
}

bool UHealthComponent::SetHealth(float NewHealth)
{
    const float Old = CurrentHealth;
    const float OldMax = MaxHealth;
    CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
    BroadcastIfChanged(Old, OldMax);
    return !FMath::IsNearlyEqual(Old, CurrentHealth);
}
