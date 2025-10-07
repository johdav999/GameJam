#include "HealthPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WorldShiftBehaviorComponent.h"
#include "ShiftPlatform.h"

AHealthPickup::AHealthPickup()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(50.f);
    RootComponent = CollisionSphere;
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    PickupMesh->SetupAttachment(RootComponent);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));
    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(PickupMesh);
    }

    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AHealthPickup::OnOverlapBegin);
}

void AHealthPickup::BeginPlay()
{
    Super::BeginPlay();
}

bool AHealthPickup::CanBeCollected() const
{
    if (!WorldShiftBehavior)
    {
        return true;
    }

    return WorldShiftBehavior->CurrentState == EPlatformState::Solid;
}

void AHealthPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!CanBeCollected() || !OtherActor)
    {
        return;
    }

    if (UHealthComponent* HealthComp = OtherActor->FindComponentByClass<UHealthComponent>())
    {
        const bool bChanged = HealthComp->Heal(HealthAmount);

        if (bChanged)
        {
            if (PickupSound)
            {
                UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
            }

            Destroy();
        }
    }
}
