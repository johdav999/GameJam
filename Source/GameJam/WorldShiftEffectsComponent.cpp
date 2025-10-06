#include "WorldShiftEffectsComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UWorldShiftEffectsComponent::UWorldShiftEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    PostProcessBlendDuration = 0.35f;
}

void UWorldShiftEffectsComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!HealthComponent)
    {
        HealthComponent = FindHealthComponentOnOwner();
    }
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

    UHealthComponent* HealthComp = HealthComponent ? HealthComponent.Get() : FindHealthComponentOnOwner();
    if (!HealthComp)
    {
        return false;
    }

    const float Damage = FMath::Abs(HealthCostPerSwitch);
    const bool bChanged = HealthComp->ApplyDamage(Damage);

    OutNewHealth = HealthComp->GetHealth();
    OutMaxHealth = HealthComp->GetMaxHealth();

    return bChanged;
}

UHealthComponent* UWorldShiftEffectsComponent::FindHealthComponentOnOwner() const
{
    if (const AActor* Owner = GetOwner())
    {
        return Owner->FindComponentByClass<UHealthComponent>();
    }

    return nullptr;
}
