#include "WorldDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WorldShiftBehaviorComponent.h"

AWorldDoor::AWorldDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;

    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    DoorMesh->SetGenerateOverlapEvents(false);
    DoorMesh->SetHiddenInGame(false);

    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));

    LastAppliedState = EWorldMaterialState::Solid;
}

void AWorldDoor::BeginPlay()
{
    Super::BeginPlay();

    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(DoorMesh);
        WorldShiftBehavior->OnMaterialStateChanged.AddDynamic(this, &AWorldDoor::OnWorldStateChanged);
        UpdateDoorState(WorldShiftBehavior->GetCurrentMaterialState());
    }
    else
    {
        UpdateDoorState(EWorldMaterialState::Solid);
    }
}

void AWorldDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->OnMaterialStateChanged.RemoveDynamic(this, &AWorldDoor::OnWorldStateChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void AWorldDoor::OnWorldStateChanged(EWorldMaterialState NewState)
{
    UpdateDoorState(NewState);
}

void AWorldDoor::UpdateDoorState(EWorldMaterialState NewState)
{
    if (!DoorMesh)
    {
        return;
    }

    switch (NewState)
    {
    case EWorldMaterialState::Solid:
        DoorMesh->SetHiddenInGame(false);
        DoorMesh->SetVisibility(true, true);
        DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
        DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        break;

    case EWorldMaterialState::Ghost:
    case EWorldMaterialState::Hidden:
    default:
        DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DoorMesh->SetVisibility(false, true);
        DoorMesh->SetHiddenInGame(true);
        break;
    }

    if (DoorSound && LastAppliedState != NewState)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DoorSound, GetActorLocation());
    }

    LastAppliedState = NewState;
}

