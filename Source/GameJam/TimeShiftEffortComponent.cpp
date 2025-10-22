#include "TimeShiftEffortComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "WorldManager.h"

UTimeShiftEffortComponent::UTimeShiftEffortComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    ClampEffort();
}

void UTimeShiftEffortComponent::BeginPlay()
{
    Super::BeginPlay();

    ClampEffort();
    OnEffortChanged.Broadcast(CurrentEffort);
}

void UTimeShiftEffortComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f)
    {
        return;
    }

    AWorldManager* Manager = AWorldManager::Get(GetWorld());
    if (!Manager)
    {
        return;
    }

    const bool bIsChaosWorld = Manager->GetCurrentWorld() == EWorldState::Chaos;
    const bool bShouldDrain = bIsChaosWorld && ShouldDepleteEffort() && DepletionRate > 0.0f;

    const float PreviousEffort = CurrentEffort;

    if (bShouldDrain)
    {
        CurrentEffort = FMath::Max(0.0f, CurrentEffort - DepletionRate * DeltaTime);
    }
    else if (!bIsChaosWorld && RecoveryRate > 0.0f && CurrentEffort < MaxEffort)
    {
        CurrentEffort = FMath::Min(MaxEffort, CurrentEffort + RecoveryRate * DeltaTime);
    }

    const bool bJustDepleted = PreviousEffort > 0.0f && CurrentEffort <= 0.0f;
    if (bJustDepleted && bIsChaosWorld)
    {
        Manager->SetWorld(EWorldState::Light);
    }

    BroadcastIfChanged(PreviousEffort);
}

bool UTimeShiftEffortComponent::ModifyEffort(float Delta)
{
    if (FMath::IsNearlyZero(Delta))
    {
        return false;
    }

    const float PreviousEffort = CurrentEffort;
    CurrentEffort = FMath::Clamp(CurrentEffort + Delta, 0.0f, MaxEffort);
    BroadcastIfChanged(PreviousEffort);
    return !FMath::IsNearlyEqual(PreviousEffort, CurrentEffort);
}

bool UTimeShiftEffortComponent::SetEffort(float NewEffort)
{
    const float PreviousEffort = CurrentEffort;
    CurrentEffort = FMath::Clamp(NewEffort, 0.0f, MaxEffort);
    BroadcastIfChanged(PreviousEffort);
    return !FMath::IsNearlyEqual(PreviousEffort, CurrentEffort);
}

void UTimeShiftEffortComponent::ClampEffort()
{
    MaxEffort = FMath::Max(0.0f, MaxEffort);
    CurrentEffort = FMath::Clamp(CurrentEffort, 0.0f, MaxEffort);
}

void UTimeShiftEffortComponent::BroadcastIfChanged(float PreviousEffort)
{
    if (!FMath::IsNearlyEqual(PreviousEffort, CurrentEffort))
    {
        OnEffortChanged.Broadcast(CurrentEffort);

        if (PreviousEffort > 0.0f && CurrentEffort <= 0.0f)
        {
            OnEffortDepleted.Broadcast();
        }
    }
}

bool UTimeShiftEffortComponent::ShouldDepleteEffort() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    if (const APawn* PawnOwner = Cast<APawn>(Owner))
    {
        if (AController* Controller = PawnOwner->GetController())
        {
            if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
            {
                return PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
            }
        }
    }
    else if (const APlayerController* PlayerController = Cast<APlayerController>(Owner))
    {
        return PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
    }

    return false;
}
