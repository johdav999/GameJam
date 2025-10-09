#include "WorldButton.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WorldShiftBehaviorComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
static const TArray<EWorldState> GAllWorldStates = {EWorldState::Light, EWorldState::Shadow, EWorldState::Chaos};
}

AWorldButton::AWorldButton()
{
    PrimaryActorTick.bCanEverTick = false;

    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    RootComponent = ButtonMesh;

    InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
    InteractionVolume->SetupAttachment(ButtonMesh);
    InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionVolume->SetGenerateOverlapEvents(true);
    InteractionVolume->SetBoxExtent(FVector(50.f, 50.f, 50.f));

    WorldShiftBehavior = CreateDefaultSubobject<UWorldShiftBehaviorComponent>(TEXT("WorldShiftBehavior"));
    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->SetTargetMesh(ButtonMesh);
    }

    bAutoPressOnOverlap = false;
    bAutoReset = true;
    ResetDelay = 1.5f;
    bCanBePressedOnce = false;
    ColorParameterName = TEXT("ButtonColor");

    DefaultVisualStyle = FWorldButtonVisualStyle(FLinearColor(0.3f, 0.3f, 0.3f, 1.f), FLinearColor(0.08f, 0.08f, 0.08f, 1.f), FVector(0.f, 0.f, -5.f));
    WorldVisualStyles.Add(EWorldState::Light, FWorldButtonVisualStyle(FLinearColor(0.95f, 0.84f, 0.32f, 1.f), FLinearColor(0.85f, 0.6f, 0.1f, 1.f), FVector(0.f, 0.f, -5.f)));
    WorldVisualStyles.Add(EWorldState::Shadow, FWorldButtonVisualStyle(FLinearColor(0.2f, 0.22f, 0.35f, 1.f), FLinearColor(0.05f, 0.05f, 0.18f, 1.f), FVector(0.f, 0.f, -5.f)));
    WorldVisualStyles.Add(EWorldState::Chaos, FWorldButtonVisualStyle(FLinearColor(0.8f, 0.3f, 0.9f, 1.f), FLinearColor(0.95f, 0.1f, 0.25f, 1.f), FVector(0.f, 0.f, -5.f)));

    SolidWorlds = {EWorldState::Light};

    bIsPressed = false;
    bHasBeenPressedOnce = false;
    bIsInteractable = true;
    CurrentVisualWorld = EWorldState::Light;
}

void AWorldButton::BeginPlay()
{
    Super::BeginPlay();

    if (ButtonMesh)
    {
        InitialButtonRelativeLocation = ButtonMesh->GetRelativeLocation();
        if (!ButtonMID)
        {
            ButtonMID = ButtonMesh->CreateDynamicMaterialInstance(0);
        }
    }

    if (InteractionVolume)
    {
        InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AWorldButton::HandleBeginOverlap);
        InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AWorldButton::HandleEndOverlap);
    }

    if (WorldShiftBehavior)
    {
        InitializeWorldBehaviorDefaults();
        WorldShiftBehavior->OnStateChanged.AddDynamic(this, &AWorldButton::HandleWorldShiftStateChanged);
        HandleWorldShiftStateChanged(WorldShiftBehavior->CurrentState, WorldShiftBehavior->CurrentWorld);
    }
    else
    {
        bIsInteractable = true;
        RefreshButtonVisuals();
    }
}

void AWorldButton::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelPendingReset();

    if (InteractionVolume)
    {
        InteractionVolume->OnComponentBeginOverlap.RemoveDynamic(this, &AWorldButton::HandleBeginOverlap);
        InteractionVolume->OnComponentEndOverlap.RemoveDynamic(this, &AWorldButton::HandleEndOverlap);
    }

    if (WorldShiftBehavior)
    {
        WorldShiftBehavior->OnStateChanged.RemoveDynamic(this, &AWorldButton::HandleWorldShiftStateChanged);
    }

    Super::EndPlay(EndPlayReason);
}

bool AWorldButton::TryPressButton(AActor* PressingActor)
{
    return InternalPress(PressingActor);
}

bool AWorldButton::IsActorOverlappingButton(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    for (const TWeakObjectPtr<AActor>& WeakActor : OverlappingActors)
    {
        if (!WeakActor.IsValid())
        {
            continue;
        }

        if (WeakActor.Get() == Actor)
        {
            return true;
        }
    }

    return false;
}

bool AWorldButton::CanBePressed() const
{
    if (!IsInteractable())
    {
        return false;
    }

    if (bIsPressed && (!bAutoReset || (bCanBePressedOnce && bHasBeenPressedOnce)))
    {
        return false;
    }

    if (bCanBePressedOnce && bHasBeenPressedOnce)
    {
        return false;
    }

    return true;
}

void AWorldButton::ResetButton()
{
    if (bCanBePressedOnce && bHasBeenPressedOnce)
    {
        return;
    }

    CancelPendingReset();

    if (!bIsPressed)
    {
        return;
    }

    bIsPressed = false;
    RefreshButtonVisuals();

    OnButtonResetDelegate.Broadcast(this);
    OnButtonReset();
}

void AWorldButton::ForceResetButton()
{
    CancelPendingReset();

    bIsPressed = false;
    bHasBeenPressedOnce = false;

    RefreshButtonVisuals();

    OnButtonResetDelegate.Broadcast(this);
    OnButtonReset();
}

void AWorldButton::InitializeWorldBehaviorDefaults()
{
    if (!WorldShiftBehavior)
    {
        return;
    }

    WorldShiftBehavior->SetTargetMesh(ButtonMesh);

    if (WorldShiftBehavior->WorldBehaviors.Num() > 0)
    {
        return;
    }

    for (EWorldState World : GAllWorldStates)
    {
        const bool bSolid = SolidWorlds.Contains(World);
        WorldShiftBehavior->WorldBehaviors.Add(World, bSolid ? EPlatformState::Solid : EPlatformState::Ghost);
    }
}

void AWorldButton::RefreshButtonVisuals()
{
    const FWorldButtonVisualStyle Style = GetVisualStyleForWorld(CurrentVisualWorld);
    ApplyVisualStyle(Style);
}

void AWorldButton::ApplyVisualStyle(const FWorldButtonVisualStyle& Style) const
{
    if (!ButtonMesh)
    {
        return;
    }

    const FVector TargetOffset = bIsPressed ? Style.PressedOffset : FVector::ZeroVector;
    ButtonMesh->SetRelativeLocation(InitialButtonRelativeLocation + TargetOffset);

    if (ButtonMID && ColorParameterName != NAME_None)
    {
        ButtonMID->SetVectorParameterValue(ColorParameterName, bIsPressed ? Style.PressedColor : Style.IdleColor);
    }
}

FWorldButtonVisualStyle AWorldButton::GetVisualStyleForWorld(EWorldState World) const
{
    if (const FWorldButtonVisualStyle* Override = WorldVisualStyles.Find(World))
    {
        return *Override;
    }

    return DefaultVisualStyle;
}

bool AWorldButton::InternalPress(AActor* PressingActor)
{
    if (!CanBePressed())
    {
        return false;
    }

    CancelPendingReset();

    bIsPressed = true;
    bHasBeenPressedOnce = true;

    RefreshButtonVisuals();
    HandlePressFeedback(PressingActor);

    OnButtonPressedDelegate.Broadcast(this, PressingActor);
    OnButtonPressed(PressingActor);

    if (bAutoReset && !(bCanBePressedOnce && bHasBeenPressedOnce))
    {
        if (ResetDelay <= 0.f)
        {
            ResetButton();
        }
        else
        {
            GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &AWorldButton::ResetButton, ResetDelay, false);
        }
    }

    return true;
}

void AWorldButton::HandlePressFeedback(AActor* PressingActor)
{
    if (PressingActor == nullptr)
    {
        // Intentional fallthrough: feedback still plays even without a valid instigator.
    }

    const FVector EffectLocation = ButtonMesh ? ButtonMesh->GetComponentLocation() : GetActorLocation();

    if (PressSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, PressSound, EffectLocation);
    }

    if (PressEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PressEffect, EffectLocation);
    }
}

void AWorldButton::CancelPendingReset()
{
    if (GetWorldTimerManager().IsTimerActive(ResetTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(ResetTimerHandle);
    }
}

void AWorldButton::HandleWorldShiftStateChanged(EPlatformState NewState, EWorldState WorldContext)
{
    CurrentVisualWorld = WorldContext;

    if (WorldShiftBehavior)
    {
        bIsInteractable = WorldShiftBehavior->IsCurrentlySolid();
    }
    else
    {
        bIsInteractable = (NewState == EPlatformState::Solid);
    }

    RefreshButtonVisuals();
}

void AWorldButton::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    for (auto It = OverlappingActors.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
        }
    }

    OverlappingActors.Add(OtherActor);

    if (bAutoPressOnOverlap && OtherActor->IsA(APawn::StaticClass()))
    {
        InternalPress(OtherActor);
    }
}

void AWorldButton::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor)
    {
        return;
    }

    for (auto It = OverlappingActors.CreateIterator(); It; ++It)
    {
        if (!It->IsValid() || It->Get() == OtherActor)
        {
            It.RemoveCurrent();
        }
    }
}

bool AWorldButton::IsInteractable() const
{
    if (!WorldShiftBehavior)
    {
        return true;
    }

    return bIsInteractable && WorldShiftBehavior->IsCurrentlySolid();
}
