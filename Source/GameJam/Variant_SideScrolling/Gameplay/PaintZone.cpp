#include "Gameplay/PaintZone.h"

#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"

APaintZone::APaintZone()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
    DecalComponent->SetupAttachment(SceneRoot);
    DecalComponent->DecalSize = FVector(32.0f, 128.0f, 128.0f);
    DecalComponent->SetFadeScreenSize(0.001f);

    OverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("ForceVolume"));
    OverlapComponent->SetupAttachment(SceneRoot);
    OverlapComponent->SetBoxExtent(FVector(120.0f));
    OverlapComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OverlapComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    OverlapComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    OverlapComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    OverlapComponent->SetGenerateOverlapEvents(true);

    bReplicates = true;
}

void APaintZone::InitializePaintZone(EForceType InForceType, const FVector& InSurfaceNormal, bool bInPermanent)
{
    ForceType = InForceType;
    SurfaceNormal = InSurfaceNormal.IsNearlyZero() ? FVector::UpVector : InSurfaceNormal.GetSafeNormal();
    bPermanent = bInPermanent;

    if (!bPermanent)
    {
        if (DecalComponent)
        {
            DecalComponent->SetFadeOut(LifeTime, FadeOutDuration, false);
        }
        SetLifeSpan(LifeTime + FadeOutDuration);
    }
    else
    {
        if (DecalComponent)
        {
            DecalComponent->SetFadeOut(0.0f, 0.0f, false);
        }
        SetLifeSpan(0.0f);
    }

    UpdateVisuals();
}

void APaintZone::BeginPlay()
{
    Super::BeginPlay();

    UpdateVisuals();
}

void APaintZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ElapsedTime += DeltaSeconds;

    const bool bForcesExpired = (!bPermanent && ElapsedTime > LifeTime);
    if (!bForcesExpired)
    {
        ApplyForces(DeltaSeconds);
    }
}

void APaintZone::UpdateVisuals()
{
    if (!DecalComponent)
    {
        return;
    }

    // Align the decal so that its forward vector matches the surface normal.
    const FMatrix RotationMatrix = FRotationMatrix::MakeFromZ(SurfaceNormal);
   
    SetActorRotation(RotationMatrix.Rotator());

    if (!DecalMID)
    {
        DecalMID = DecalComponent->CreateDynamicMaterialInstance();
    }

    if (DecalMID)
    {
        FLinearColor DesiredColor = PushColor;
        switch (ForceType)
        {
        case EForceType::Pull:
            DesiredColor = PullColor;
            break;
        case EForceType::Gravity:
            DesiredColor = GravityColor;
            break;
        case EForceType::Push:
        default:
            DesiredColor = PushColor;
            break;
        }

        DecalMID->SetVectorParameterValue(ColorParameterName, DesiredColor);
    }
}

void APaintZone::ApplyForces(float DeltaSeconds)
{
    if (!OverlapComponent)
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    OverlapComponent->GetOverlappingActors(OverlappingActors);

    if (OverlappingActors.IsEmpty())
    {
        return;
    }

    const float Strength = (ForceType == EForceType::Gravity) ? GravityStrength : ForceStrength;

    FVector ForceDirection = SurfaceNormal;
    if (ForceType == EForceType::Pull || ForceType == EForceType::Gravity)
    {
        ForceDirection *= -1.0f;
    }

    const FVector Acceleration = ForceDirection * Strength;

    for (AActor* Actor : OverlappingActors)
    {
        if (!Actor || Actor == this)
        {
            continue;
        }

        bool bAppliedForce = false;

        // Handle characters explicitly.
        if (ACharacter* Character = Cast<ACharacter>(Actor))
        {
            if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
            {
                MovementComponent->AddForce(Acceleration * MovementComponent->Mass);
                bAppliedForce = true;
            }
        }

        // Apply force to any simulating components the actor owns.
        TArray<UPrimitiveComponent*> PrimitiveComponents;
        Actor->GetComponents(PrimitiveComponents);
        for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
        {
            if (!PrimitiveComponent || !PrimitiveComponent->IsSimulatingPhysics())
            {
                continue;
            }

            PrimitiveComponent->AddForce(Acceleration * PrimitiveComponent->GetMass(), NAME_None, true);
            bAppliedForce = true;
        }

        if (!bAppliedForce && ForceType == EForceType::Gravity)
        {
            // For non-physics actors (like pawns) that lack simulation, apply a small impulse to mimic pull.
            if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
            {
                if (RootPrimitive->IsSimulatingPhysics())
                {
                    // Already handled in loop above.
                    continue;
                }

                RootPrimitive->AddForce(Acceleration * RootPrimitive->GetMass(), NAME_None, true);
            }
        }
    }
}

