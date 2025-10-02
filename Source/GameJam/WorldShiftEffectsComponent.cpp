#include "WorldShiftEffectsComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

namespace
{
    /** Helper list of common function names used across health components for applying delta values. */
    static const TArray<FName> ModifyHealthFunctionNames = {
        TEXT("ModifyHealth"),
        TEXT("ApplyHealthChange"),
        TEXT("AdjustHealth"),
        TEXT("ChangeHealth"),
        TEXT("AddHealth"),
        TEXT("ReduceHealth"),
        TEXT("SubtractHealth"),
        TEXT("ConsumeHealth"),
        TEXT("SetHealthDelta"),
        TEXT("SetHealthChange"),
        TEXT("AdjustHitPoints")
    };

    /** Helper list of function names that typically inflict damage on health components. */
    static const TArray<FName> DamageHealthFunctionNames = {
        TEXT("TakeDamage"),
        TEXT("DealDamage"),
        TEXT("ApplyDamage"),
        TEXT("InflictDamage"),
        TEXT("ReceiveDamage")
    };

    /** Helper list of functions that return the current health value. */
    static const TArray<FName> CurrentHealthFunctionNames = {
        TEXT("GetCurrentHealth"),
        TEXT("GetHealth"),
        TEXT("GetHealthValue"),
        TEXT("GetRemainingHealth"),
        TEXT("GetHealthPoints"),
        TEXT("GetCurrentHP"),
        TEXT("GetHP")
    };

    /** Helper list of functions that return the maximum health value. */
    static const TArray<FName> MaxHealthFunctionNames = {
        TEXT("GetMaxHealth"),
        TEXT("GetMaximumHealth"),
        TEXT("GetHealthCapacity"),
        TEXT("GetTotalHealth"),
        TEXT("GetMaxHP"),
        TEXT("GetHPMax")
    };

    /** Helper list of common property names storing current health. */
    static const TArray<FName> CurrentHealthPropertyNames = {
        TEXT("CurrentHealth"),
        TEXT("Health"),
        TEXT("HealthValue"),
        TEXT("RemainingHealth"),
        TEXT("HealthPoints"),
        TEXT("CurrentHP"),
        TEXT("HP")
    };

    /** Helper list of common property names storing maximum health. */
    static const TArray<FName> MaxHealthPropertyNames = {
        TEXT("MaxHealth"),
        TEXT("MaximumHealth"),
        TEXT("HealthCapacity"),
        TEXT("TotalHealth"),
        TEXT("MaxHP"),
        TEXT("HPMax")
    };
}

UWorldShiftEffectsComponent::UWorldShiftEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    HealthCostPerSwitch = 5.0f;
    PostProcessBlendDuration = 0.35f;
}

void UWorldShiftEffectsComponent::TriggerWorldShiftEffects(EWorldState NewWorld)
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    OnWorldShiftTriggered.Broadcast(NewWorld);
    OnSpecialWorldEffect(NewWorld);

    if (USoundBase* const* SoundPtr = SwitchSounds.Find(NewWorld))
    {
        if (USoundBase* Sound = *SoundPtr)
        {
            UGameplayStatics::PlaySoundAtLocation(this, Sound, Owner->GetActorLocation(), Owner->GetActorRotation());
        }
    }

    if (UNiagaraSystem* const* ParticlePtr = SwitchParticles.Find(NewWorld))
    {
        if (UNiagaraSystem* ParticleSystem = *ParticlePtr)
        {
            if (UWorld* World = GetWorld())
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ParticleSystem, Owner->GetActorLocation(), Owner->GetActorRotation());
            }
        }
    }

    if (const FLinearColor* FlashColor = SwitchColors.Find(NewWorld))
    {
        StartPostProcessFlash(*FlashColor);
    }

    float NewHealth = 0.0f;
    float MaxHealth = 0.0f;
    if (ApplyHealthCost(NewHealth, MaxHealth))
    {
        OnHealthDrained.Broadcast(NewHealth, MaxHealth);
    }
}

void UWorldShiftEffectsComponent::StartPostProcessFlash(FLinearColor FlashColor)
{
    FlashColor.A = 1.0f;

    const float FadeDuration = FMath::Max(PostProcessBlendDuration, KINDA_SMALL_NUMBER);

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
        {
            CameraManager->StartCameraFade(1.0f, 0.0f, FadeDuration, FlashColor, false, false);
        }
    }
}

bool UWorldShiftEffectsComponent::ApplyHealthCost(float& OutNewHealth, float& OutMaxHealth)
{
    OutNewHealth = 0.0f;
    OutMaxHealth = 0.0f;

    if (HealthCostPerSwitch <= 0.0f)
    {
        return false;
    }

    UActorComponent* HealthComponent = ResolveHealthComponent();
    if (!HealthComponent)
    {
        return false;
    }

    const float Delta = -FMath::Abs(HealthCostPerSwitch);
    bool bHealthModified = TryInvokeHealthDelta(HealthComponent, Delta);

    if (!bHealthModified)
    {
        for (const FName& PropertyName : CurrentHealthPropertyNames)
        {
            if (FProperty* Property = HealthComponent->GetClass()->FindPropertyByName(PropertyName))
            {
                if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
                {
                    float CurrentValue = FloatProperty->GetPropertyValue_InContainer(HealthComponent);
                    const float NewValue = FMath::Max(0.0f, CurrentValue + Delta);
                    FloatProperty->SetPropertyValue_InContainer(HealthComponent, NewValue);
                    bHealthModified = true;
                    break;
                }
            }
        }
    }

    if (!bHealthModified)
    {
        return false;
    }

    TryGetHealthValues(HealthComponent, OutNewHealth, OutMaxHealth);
    return true;
}

UActorComponent* UWorldShiftEffectsComponent::ResolveHealthComponent() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    Owner->GetComponents(Components);
    for (UActorComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        const FString ComponentName = Component->GetName();
        const FString ClassName = Component->GetClass()->GetName();

        if (ComponentName.Contains(TEXT("Health"), ESearchCase::IgnoreCase) || ClassName.Contains(TEXT("Health"), ESearchCase::IgnoreCase))
        {
            return Component;
        }
    }

    return nullptr;
}

bool UWorldShiftEffectsComponent::TryInvokeHealthDelta(UActorComponent* HealthComponent, float Delta) const
{
    if (!HealthComponent)
    {
        return false;
    }

    struct FSingleFloatParam
    {
        float Amount;
    };

    const auto InvokeWithValue = [HealthComponent](const TArray<FName>& FunctionNames, float Value) -> bool
    {
        for (const FName& FunctionName : FunctionNames)
        {
            if (UFunction* Function = HealthComponent->FindFunction(FunctionName))
            {
                FSingleFloatParam Params{Value};
                HealthComponent->ProcessEvent(Function, &Params);
                return true;
            }
        }
        return false;
    };

    if (InvokeWithValue(ModifyHealthFunctionNames, Delta))
    {
        return true;
    }

    if (InvokeWithValue(DamageHealthFunctionNames, -Delta))
    {
        return true;
    }

    return false;
}

bool UWorldShiftEffectsComponent::TryGetHealthValues(UActorComponent* HealthComponent, float& OutCurrentHealth, float& OutMaxHealth) const
{
    bool bHasCurrent = TryGetHealthValueFromFunction(HealthComponent, CurrentHealthFunctionNames, OutCurrentHealth) ||
                       TryGetHealthValueFromProperty(HealthComponent, CurrentHealthPropertyNames, OutCurrentHealth);

    bool bHasMax = TryGetHealthValueFromFunction(HealthComponent, MaxHealthFunctionNames, OutMaxHealth) ||
                   TryGetHealthValueFromProperty(HealthComponent, MaxHealthPropertyNames, OutMaxHealth);

    if (!bHasMax)
    {
        // If we cannot find a max health value, fall back to current health as both values to keep UI stable.
        OutMaxHealth = OutCurrentHealth;
    }

    if (!bHasCurrent)
    {
        OutCurrentHealth = 0.0f;
    }

    return bHasCurrent || bHasMax;
}

bool UWorldShiftEffectsComponent::TryGetHealthValueFromFunction(UActorComponent* HealthComponent, const TArray<FName>& FunctionNames, float& OutValue) const
{
    if (!HealthComponent)
    {
        return false;
    }

    for (const FName& FunctionName : FunctionNames)
    {
        if (UFunction* Function = HealthComponent->FindFunction(FunctionName))
        {
            if (Function->NumParms == 0 && Function->ParmsSize == 0)
            {
                if (CastField<FFloatProperty>(Function->GetReturnProperty()) != nullptr)
                {
                    struct FFloatReturn
                    {
                        float ReturnValue;
                    } Params{0.0f};

                    HealthComponent->ProcessEvent(Function, &Params);
                    OutValue = Params.ReturnValue;
                    return true;
                }
            }
        }
    }

    return false;
}

bool UWorldShiftEffectsComponent::TryGetHealthValueFromProperty(UActorComponent* HealthComponent, const TArray<FName>& PropertyNames, float& OutValue) const
{
    if (!HealthComponent)
    {
        return false;
    }

    for (const FName& PropertyName : PropertyNames)
    {
        if (FProperty* Property = HealthComponent->GetClass()->FindPropertyByName(PropertyName))
        {
            if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
            {
                OutValue = FloatProperty->GetPropertyValue_InContainer(HealthComponent);
                return true;
            }
        }
    }

    return false;
}
