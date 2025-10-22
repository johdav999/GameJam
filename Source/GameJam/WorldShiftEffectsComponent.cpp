#include "WorldShiftEffectsComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UWorldShiftEffectsComponent::UWorldShiftEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

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
                UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                    World,
                    ParticleSystem,
                    Owner->GetActorLocation(),
                    Owner->GetActorRotation(),
                    FVector(1.f),
                    true,
                    true,
                    ENCPoolMethod::AutoRelease);

                if (NiagaraComp)
                {
                    NiagaraComp->OnSystemFinished.AddDynamic(this, &UWorldShiftEffectsComponent::OnNiagaraEffectFinished);
                }
            }
        }
    }

    if (const FLinearColor* FlashColor = SwitchColors.Find(NewWorld))
    {
        StartPostProcessFlash(*FlashColor);
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

void UWorldShiftEffectsComponent::OnNiagaraEffectFinished(UNiagaraComponent* FinishedComponent)
{
    if (!FinishedComponent)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Niagara effect finished: %s"), *FinishedComponent->GetName());
}
