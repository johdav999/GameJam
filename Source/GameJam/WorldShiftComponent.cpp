#include "WorldShiftComponent.h"
#include "WorldManager.h"
#include "GameFramework/Actor.h"

#include "Components/PrimitiveComponent.h"
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
	UE_LOG(LogTemp, Warning, TEXT("WorldShiftComponent handling world shift to: %d"), (int32)NewWorld); 
    OnWorldShift(NewWorld);
}
void UWorldShiftComponent::OnWorldShift(EWorldState NewWorld)
{

}