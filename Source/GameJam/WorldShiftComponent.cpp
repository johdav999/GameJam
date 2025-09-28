#include "WorldShiftComponent.h"
#include "WorldManager.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"

UWorldShiftComponent::UWorldShiftComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    VisibleInWorlds.Add(EWorldState::Light);
}

void UWorldShiftComponent::BeginPlay()
{
    Super::BeginPlay();

    BindToWorldManager();

    if (CachedWorldManager.IsValid())
    {
        OnWorldShift(CachedWorldManager->GetCurrentWorld());
    }
}

void UWorldShiftComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CachedWorldManager.IsValid())
    {
        CachedWorldManager->OnWorldShifted.RemoveDynamic(this, &UWorldShiftComponent::HandleWorldShift);
    }

    Super::EndPlay(EndPlayReason);
}

void UWorldShiftComponent::OnRegister()
{
    Super::OnRegister();

    BindToWorldManager();
}

void UWorldShiftComponent::BindToWorldManager()
{
    if (CachedWorldManager.IsValid())
    {
        return;
    }

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        CachedWorldManager = Manager;
        Manager->OnWorldShifted.AddDynamic(this, &UWorldShiftComponent::HandleWorldShift);
    }
}

void UWorldShiftComponent::HandleWorldShift(EWorldState NewWorld)
{
    OnWorldShift(NewWorld);
}

void UWorldShiftComponent::OnWorldShift(EWorldState NewWorld)
{
    const bool bShouldBeVisible = VisibleInWorlds.Contains(NewWorld);

    if (AActor* Owner = GetOwner())
    {
        Owner->SetActorHiddenInGame(!bShouldBeVisible);
        Owner->SetActorEnableCollision(bShouldBeVisible);

        TArray<UActorComponent*> PrimitiveComponents = Owner->GetComponentsByClass(UPrimitiveComponent::StaticClass());
        for (UActorComponent* Component : PrimitiveComponents)
        {
            if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
            {
                Primitive->SetHiddenInGame(!bShouldBeVisible);
                Primitive->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
            }
        }
    }
}
