#include "HintNPCSpawner.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "HintNPCCharacter.h"
#include "Kismet/GameplayStatics.h"

AHintNPCSpawner::AHintNPCSpawner()
    : NPCClass(nullptr)
    , bSpawnOnce(true)
    , SpawnOffset(FTransform::Identity)
    , bHasSpawned(false)
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
    TriggerVolume->SetGenerateOverlapEvents(true);
    TriggerVolume->SetBoxExtent(FVector(100.0f));

    RootComponent = TriggerVolume;

    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AHintNPCSpawner::HandleTriggerOverlap);
}

void AHintNPCSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (!TriggerVolume)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' is missing its trigger volume."), *GetName());
    }
}

void AHintNPCSpawner::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !NPCClass)
    {
        return;
    }

    if (bSpawnOnce && bHasSpawned)
    {
        return;
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor != PlayerCharacter)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform BaseTransform = GetActorTransform();

    const FVector SpawnLocation = BaseTransform.TransformPosition(SpawnOffset.GetLocation());
    const FQuat SpawnRotation = SpawnOffset.GetRotation() * BaseTransform.GetRotation();
    const FVector SpawnScale = BaseTransform.GetScale3D() * SpawnOffset.GetScale3D();

    const FTransform SpawnTransform(SpawnRotation, SpawnLocation, SpawnScale);

    if (AHintNPCCharacter* SpawnedNPC = World->SpawnActor<AHintNPCCharacter>(NPCClass, SpawnTransform))
    {
        if (bSpawnOnce)
        {
            bHasSpawned = true;
        }
    }
}
