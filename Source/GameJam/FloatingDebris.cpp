#include "FloatingDebris.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

AFloatingDebris::AFloatingDebris()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
    SpawnBounds->SetupAttachment(SceneRoot);
    SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnBounds->SetGenerateOverlapEvents(false);
    SpawnBounds->SetBoxExtent(FVector(100.f));
    SpawnBounds->SetHiddenInGame(true);
    SpawnBounds->SetCanEverAffectNavigation(false);
    SpawnBounds->bDrawOnlyIfSelected = true;

    DebrisCount = 3;
    bEnableFloating = true;
    FloatingAmplitude = FVector(50.f, 50.f, 75.f);
    FloatingSpeed = FVector(0.3f, 0.27f, 0.35f);
    BaseRotationSpeedDeg = 10.f;
    RotationSpeedVariance = 5.f;
    RandomPhaseRange = 2.f * PI;
    RotationAxisConeHalfAngle = 180.f;

    RunningTime = 0.f;
}

void AFloatingDebris::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    RunningTime = 0.f;
    SpawnDebrisMeshes();
}

void AFloatingDebris::BeginPlay()
{
    Super::BeginPlay();

    SpawnDebrisMeshes();
}

void AFloatingDebris::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearSpawnedDebris();

    Super::EndPlay(EndPlayReason);
}

void AFloatingDebris::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RunningTime += DeltaSeconds;

    if (!bEnableFloating)
    {
        return;
    }

    const float TimeSeconds = RunningTime;
    const FVector AngularSpeed = FloatingSpeed * (2.f * PI);

    for (FDebrisInstanceData& Instance : SpawnedDebris)
    {
        if (!IsValid(Instance.MeshComponent))
        {
            continue;
        }

        const FVector Offset(
            FMath::Sin(TimeSeconds * AngularSpeed.X + Instance.PhaseOffset) * FloatingAmplitude.X,
            FMath::Sin(TimeSeconds * AngularSpeed.Y + Instance.PhaseOffset) * FloatingAmplitude.Y,
            FMath::Sin(TimeSeconds * AngularSpeed.Z + Instance.PhaseOffset) * FloatingAmplitude.Z);

        Instance.MeshComponent->SetRelativeLocation(Instance.InitialRelativeLocation + Offset);

        const float DeltaAngle = Instance.RotationSpeedDeg * DeltaSeconds;
        if (!Instance.RotationAxis.IsNearlyZero())
        {
            const FQuat DeltaQuat = FQuat(Instance.RotationAxis.GetSafeNormal(), FMath::DegreesToRadians(DeltaAngle));
            const FQuat NewRotation = DeltaQuat * Instance.MeshComponent->GetRelativeRotation().Quaternion();
            Instance.MeshComponent->SetRelativeRotation(NewRotation);
        }
    }
}

void AFloatingDebris::SetFloatingEnabled(bool bInEnabled)
{
    bEnableFloating = bInEnabled;
}

void AFloatingDebris::RespawnDebris()
{
    SpawnDebrisMeshes();
}

void AFloatingDebris::SpawnDebrisMeshes()
{
    ClearSpawnedDebris();

    if (DebrisCount <= 0 || DebrisMeshes.Num() == 0)
    {
        return;
    }

    SpawnedDebris.Reserve(DebrisCount);

    const FVector Extent = SpawnBounds ? SpawnBounds->GetScaledBoxExtent() : FVector::ZeroVector;

    for (int32 Index = 0; Index < DebrisCount; ++Index)
    {
        const int32 MeshIndex = DebrisMeshes.Num() > 0 ? FMath::RandRange(0, DebrisMeshes.Num() - 1) : INDEX_NONE;
        UStaticMesh* SelectedMesh = MeshIndex != INDEX_NONE ? DebrisMeshes[MeshIndex] : nullptr;
        if (!SelectedMesh)
        {
            continue;
        }

        const FString ComponentName = FString::Printf(TEXT("DebrisMesh_%d"), Index);
        UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this, FName(*ComponentName));
        if (!MeshComponent)
        {
            continue;
        }

        MeshComponent->SetStaticMesh(SelectedMesh);
        MeshComponent->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        MeshComponent->SetGenerateOverlapEvents(false);
        MeshComponent->SetupAttachment(SceneRoot);
        MeshComponent->RegisterComponent();
        MeshComponent->SetCanEverAffectNavigation(false);

        const FVector RandomOffset(
            FMath::FRandRange(-Extent.X, Extent.X),
            FMath::FRandRange(-Extent.Y, Extent.Y),
            FMath::FRandRange(-Extent.Z, Extent.Z));

        MeshComponent->SetRelativeLocation(RandomOffset);
        MeshComponent->SetRelativeRotation(FRotator::MakeFromEuler(FVector(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f))));

        FDebrisInstanceData& Instance = SpawnedDebris.AddDefaulted_GetRef();
        Instance.MeshComponent = MeshComponent;
        Instance.PhaseOffset = FMath::FRandRange(-RandomPhaseRange, RandomPhaseRange);
        Instance.InitialRelativeLocation = RandomOffset;

        const float ConeAngle = FMath::Clamp(RotationAxisConeHalfAngle, 0.f, 180.f);
        FVector RandomAxis = FVector::UpVector;
        if (ConeAngle >= 179.9f)
        {
            RandomAxis = FMath::VRand();
        }
        else if (ConeAngle > 0.f)
        {
            const float ConeAngleRadians = FMath::DegreesToRadians(ConeAngle);
            RandomAxis = FMath::VRandCone(FVector::UpVector, ConeAngleRadians);
        }

        Instance.RotationAxis = RandomAxis.GetSafeNormal();
        if (Instance.RotationAxis.IsNearlyZero())
        {
            Instance.RotationAxis = FVector::UpVector;
        }

        Instance.RotationSpeedDeg = BaseRotationSpeedDeg + FMath::FRandRange(-RotationSpeedVariance, RotationSpeedVariance);
    }

    RunningTime = 0.f;
}

void AFloatingDebris::ClearSpawnedDebris()
{
    for (FDebrisInstanceData& Instance : SpawnedDebris)
    {
        if (Instance.MeshComponent)
        {
            Instance.MeshComponent->DestroyComponent();
        }
    }

    SpawnedDebris.Empty();
}

#if WITH_EDITOR
void AFloatingDebris::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.MemberProperty)
    {
        const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AFloatingDebris, DebrisCount))
        {
            DebrisCount = FMath::Max(0, DebrisCount);
        }
    }
}
#endif
