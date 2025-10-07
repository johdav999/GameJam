#include "WorldDoor.h"

#include "WorldManager.h"
#include "WorldShiftComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AWorldDoor::AWorldDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;

    WorldShiftComponent = CreateDefaultSubobject<UWorldShiftComponent>(TEXT("WorldShiftComponent"));

    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DoorMesh->SetGenerateOverlapEvents(false);

    bAnimateOnToggle = true;
    bIsCurrentlyVisible = true;
    bHasInitialized = false;
}

void AWorldDoor::BeginPlay()
{
    Super::BeginPlay();

    if (WorldShiftComponent)
    {
        WorldShiftComponent->OnWorldShifted.AddDynamic(this, &AWorldDoor::HandleWorldShift);
    }

    if (AWorldManager* Manager = AWorldManager::Get(GetWorld()))
    {
        HandleWorldShift(Manager->GetCurrentWorld());
    }
    else
    {
        const bool bShouldBeVisible = VisibleInWorlds.Contains(EWorldState::Light);
        SetDoorState(bShouldBeVisible);
    }
}

void AWorldDoor::HandleWorldShift(EWorldState NewWorld)
{
    const bool bShouldBeVisible = VisibleInWorlds.Contains(NewWorld);
    SetDoorState(bShouldBeVisible);
}

void AWorldDoor::SetDoorState(bool bShouldBeVisible)
{
    if (bHasInitialized && bIsCurrentlyVisible == bShouldBeVisible)
    {
        return;
    }

    bHasInitialized = true;
    bIsCurrentlyVisible = bShouldBeVisible;

    if (DoorMesh)
    {
        DoorMesh->SetHiddenInGame(!bShouldBeVisible);
        DoorMesh->SetCollisionEnabled(bShouldBeVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (bAnimateOnToggle)
    {
        PlayDoorAnimation(bShouldBeVisible);
    }

    if (bShouldBeVisible && OpenSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
    }
    else if (!bShouldBeVisible && CloseSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, CloseSound, GetActorLocation());
    }
}

void AWorldDoor::PlayDoorAnimation(bool bOpening)
{
    if (!DoorMesh)
    {
        return;
    }

    const FRotator TargetRotation = bOpening ? FRotator(0.f, 90.f, 0.f) : FRotator::ZeroRotator;
    DoorMesh->SetRelativeRotation(TargetRotation);
}
