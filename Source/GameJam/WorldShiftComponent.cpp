#include "WorldShiftComponent.h"

#include "GameFramework/Actor.h"
#include "WorldManager.h"

UWorldShiftComponent::UWorldShiftComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWorldShiftComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AWorldManager* Manager = AWorldManager::GetWorldManager(this))
    {
        Manager->OnWorldShifted.AddDynamic(this, &UWorldShiftComponent::HandleWorldShift);
        HandleWorldShift(Manager->GetCurrentWorld());
    }
}

void UWorldShiftComponent::HandleWorldShift(EWorldState NewWorld)
{
    bool bShouldBeVisible = false;
    switch (NewWorld)
    {
    case EWorldState::Light:
        bShouldBeVisible = bVisibleInLight;
        break;
    case EWorldState::Shadow:
        bShouldBeVisible = bVisibleInShadow;
        break;
    case EWorldState::Dream:
        bShouldBeVisible = bVisibleInDream;
        break;
    default:
        break;
    }

    UpdateActorState(bShouldBeVisible);
}

void UWorldShiftComponent::UpdateActorState(bool bShouldBeActive) const
{
    if (AActor* Owner = GetOwner())
    {
        Owner->SetActorHiddenInGame(!bShouldBeActive);
        if (bAffectsCollision)
        {
            Owner->SetActorEnableCollision(bShouldBeActive);
        }
    }
}
