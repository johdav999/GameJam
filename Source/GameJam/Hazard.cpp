#include "Hazard.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "ShiftPlatform.h"
#include "WorldShiftBehaviorComponent.h"
#include "Engine/EngineTypes.h"

AHazard::AHazard()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetGenerateOverlapEvents(true);
	
    HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
    HazardMesh->SetupAttachment(RootComponent);
    HazardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HazardMesh->SetupAttachment(RootComponent);
    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));


    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AHazard::OnOverlapBegin);
    CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AHazard::OnOverlapEnd);
}

void AHazard::BeginPlay()
{
    Super::BeginPlay();

    bool bShouldBeActive = bIsActive;

    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(HazardMesh);
        WorldShiftBehavior->OnStateChanged.AddDynamic(this, &AHazard::HandleWorldStateChanged);
        bShouldBeActive = WorldShiftBehavior->IsCurrentlySolid() || IsSolidState(WorldShiftBehavior->CurrentState);
    }

    if (ActiveEffect)
    {
        ActiveEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(ActiveEffect, RootComponent, NAME_None, FVector::ZeroVector,
            FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false);
    }

    if (ActiveSound)
    {
        ActiveAudioComponent = NewObject<UAudioComponent>(this);
        ActiveAudioComponent->bAutoActivate = false;
        ActiveAudioComponent->bAutoDestroy = false;
        ActiveAudioComponent->SetSound(ActiveSound);
        ActiveAudioComponent->SetupAttachment(RootComponent);
        ActiveAudioComponent->RegisterComponent();
    }

    bIsActive = !bShouldBeActive;
    UpdateActivation(bShouldBeActive);

    if (bContinuousDamage)
    {
        GetWorldTimerManager().SetTimer(DamageTickHandle, this, &AHazard::ApplyContinuousDamage, 1.0f, true);
    }
}

void AHazard::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!bIsActive || !OtherActor || OtherActor == this)
    {
        return;
    }

    if (bContinuousDamage)
    {
        OverlappingActors.Add(OtherActor);
    }
    else
    {
        DealDamageToActor(OtherActor);
    }
}

void AHazard::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor)
    {
        OverlappingActors.Remove(OtherActor);
    }
}

void AHazard::HandleWorldStateChanged(EPlatformState NewState, EWorldState WorldContext)
{
    UpdateActivation(IsSolidState(NewState));
}

void AHazard::ApplyContinuousDamage()
{
    if (!bIsActive)
    {
        return;
    }

    for (auto It = OverlappingActors.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
            continue;
        }

        if (AActor* Target = It->Get())
        {
            DealDamageToActor(Target);
        }
    }
}

void AHazard::DealDamageToActor(AActor* Target)
{
    if (!Target || Target == this || DamageAmount <= 0.f)
    {
        return;
    }

    if (UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>())
    {
        Health->ApplyDamage(DamageAmount);
    }
}

void AHazard::UpdateActivation(bool bNewActive)
{
    const bool bWasActive = bIsActive;
    bIsActive = bNewActive;

    if (CollisionBox)
    {
        CollisionBox->SetCollisionEnabled(bIsActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        CollisionBox->SetGenerateOverlapEvents(bIsActive);
    }

    if (!bIsActive)
    {
        OverlappingActors.Empty();

        if (ActiveEffectComponent && ActiveEffectComponent->IsActive())
        {
            ActiveEffectComponent->Deactivate();
        }

        if (ActiveAudioComponent && ActiveAudioComponent->IsPlaying())
        {
            ActiveAudioComponent->FadeOut(0.1f, 0.f);
        }

        return;
    }

    if (!ActiveEffectComponent && ActiveEffect)
    {
        ActiveEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(ActiveEffect, RootComponent, NAME_None, FVector::ZeroVector,
            FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false);
    }

    if (ActiveEffectComponent && !ActiveEffectComponent->IsActive())
    {
        ActiveEffectComponent->Activate(true);
    }

    if (ActiveAudioComponent && !ActiveAudioComponent->IsPlaying())
    {
        if (bWasActive)
        {
            ActiveAudioComponent->Play();
        }
        else
        {
            ActiveAudioComponent->FadeIn(0.05f, 1.f);
        }
    }

    if (!bWasActive && CollisionBox)
    {
        TArray<AActor*> CurrentlyOverlapping;
        CollisionBox->GetOverlappingActors(CurrentlyOverlapping);
        for (AActor* Actor : CurrentlyOverlapping)
        {
            if (!IsValid(Actor) || Actor == this)
            {
                continue;
            }

            if (bContinuousDamage)
            {
                OverlappingActors.Add(Actor);
            }
            else
            {
                DealDamageToActor(Actor);
            }
        }
    }
}

bool AHazard::IsSolidState(EPlatformState State) const
{
    return State == EPlatformState::Solid || State == EPlatformState::TimedSolid;
}
