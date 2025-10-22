#include "Hints/HintNPCSpawner.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "HintNPCCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "WorldManager.h"

namespace
{
    constexpr float DefaultTargetAcceptanceRadius = 5.0f;
    constexpr float DreamWorldOverrideDuration = 3.0f;
}

AHintNPCSpawner::AHintNPCSpawner()
    : NPCClass(nullptr)
    , bSpawnOnce(true)
    , SpawnOffset(FTransform::Identity)
    , SpawnDelay(3.0f)
    , bHasSpawned(false)
    , ActiveMoveRequestID(FAIRequestID::InvalidRequest)
    , PreviousWorldState(EWorldState::Light)
    , bIsWorldOverrideActive(false)
    , bIsMovementDelayActive(false)
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
    TriggerVolume->SetGenerateOverlapEvents(true);
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetBoxExtent(FVector(100.0f));
    TriggerVolume->SetHiddenInGame(true);
    TriggerVolume->SetCanEverAffectNavigation(false);

    RootComponent = TriggerVolume;

    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AHintNPCSpawner::HandleTriggerOverlap);

    TargetLocation = CreateDefaultSubobject<UBoxComponent>(TEXT("TargetLocation"));
    TargetLocation->SetupAttachment(RootComponent);
    TargetLocation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TargetLocation->SetGenerateOverlapEvents(false);
    TargetLocation->SetHiddenInGame(true);
    TargetLocation->SetCanEverAffectNavigation(false);
    TargetLocation->SetBoxExtent(FVector(25.0f));
    TargetLocation->ShapeColor = FColor::Cyan;
    TargetLocation->bIsEditorOnly = true;
}

void AHintNPCSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (!TriggerVolume)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' is missing its trigger volume."), *GetName());
    }

    if (!TargetLocation)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' is missing its target location marker."), *GetName());
    }
}

void AHintNPCSpawner::HandleTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !TriggerVolume || !TargetLocation || !NPCClass)
    {
        return;
    }

    if (bSpawnOnce && bHasSpawned)
    {
        return;
    }

    if (ActiveNPC.IsValid())
    {
        return;
    }

    if (bIsMovementDelayActive)
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

    FTimerManager& TimerManager = World->GetTimerManager();

    if (AWorldManager* WorldManager = AWorldManager::Get(World))
    {
        const EWorldState CurrentWorld = WorldManager->GetCurrentWorld();

        if (!bIsWorldOverrideActive)
        {
            PreviousWorldState = CurrentWorld;
        }

        bIsWorldOverrideActive = true;

        UE_LOG(LogTemp, Log, TEXT("Switching to Dream World..."));

        WorldManager->SetWorld(EWorldState::Shadow);

        TimerManager.ClearTimer(ShadowWorldTimerHandle);
        TimerManager.SetTimer(ShadowWorldTimerHandle, this, &AHintNPCSpawner::RestoreWorldState, DreamWorldOverrideDuration, false);
    }

    const FTransform BaseTransform = GetActorTransform();

    const FVector SpawnLocation = BaseTransform.TransformPosition(SpawnOffset.GetLocation());
    const FQuat SpawnRotation = SpawnOffset.GetRotation() * BaseTransform.GetRotation();
    const FVector SpawnScale = BaseTransform.GetScale3D() * SpawnOffset.GetScale3D();

    const FTransform SpawnTransform(SpawnRotation, SpawnLocation, SpawnScale);

    AHintNPCCharacter* SpawnedNPC = World->SpawnActor<AHintNPCCharacter>(NPCClass, SpawnTransform);
    if (!SpawnedNPC)
    {
        return;
    }

    if (bSpawnOnce)
    {
        bHasSpawned = true;
    }

    ActiveNPC = SpawnedNPC;

    AAIController* AIController = Cast<AAIController>(SpawnedNPC->GetController());
    if (!AIController)
    {
        SpawnedNPC->SpawnDefaultController();
        AIController = Cast<AAIController>(SpawnedNPC->GetController());
    }

    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' could not find or create an AI controller for spawned NPC '%s'."), *GetName(), *SpawnedNPC->GetName());
        SpawnedNPC->Destroy();
        ActiveNPC.Reset();
        return;
    }

    ActiveNPCController = AIController;

    SpawnedNPC->SetActorHiddenInGame(true);
    SpawnedNPC->SetActorEnableCollision(false);

    UE_LOG(LogTemp, Log, TEXT("Player overlapped NPC spawner, waiting 3 seconds before NPC moves..."));

    TimerManager.ClearTimer(MovementDelayTimerHandle);
    bIsMovementDelayActive = true;
    TimerManager.SetTimer(MovementDelayTimerHandle, this, &AHintNPCSpawner::BeginNPCMovement, SpawnDelay, false);
}

void AHintNPCSpawner::BeginNPCMovement()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        bIsMovementDelayActive = false;
        return;
    }

    FTimerManager& TimerManager = World->GetTimerManager();
    TimerManager.ClearTimer(MovementDelayTimerHandle);
    bIsMovementDelayActive = false;

    if (!TargetLocation)
    {
        CleanupActiveNPC();
        return;
    }

    if (!ActiveNPC.IsValid())
    {
        CleanupActiveNPC();
        return;
    }

    AHintNPCCharacter* SpawnedNPC = ActiveNPC.Get();
    if (!SpawnedNPC)
    {
        CleanupActiveNPC();
        return;
    }

    if (!ActiveNPCController.IsValid())
    {
        AAIController* ExistingController = Cast<AAIController>(SpawnedNPC->GetController());
        if (!ExistingController)
        {
            SpawnedNPC->SpawnDefaultController();
            ExistingController = Cast<AAIController>(SpawnedNPC->GetController());
        }

        if (!ExistingController)
        {
            UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' could not find or create an AI controller for spawned NPC '%s'."), *GetName(), *SpawnedNPC->GetName());
            SpawnedNPC->Destroy();
            CleanupActiveNPC();
            return;
        }

        ActiveNPCController = ExistingController;
    }

    AAIController* AIController = ActiveNPCController.Get();
    if (!AIController)
    {
        CleanupActiveNPC();
        return;
    }

    SpawnedNPC->SetActorHiddenInGame(false);
    SpawnedNPC->SetActorEnableCollision(true);

    UE_LOG(LogTemp, Log, TEXT("NPC starting movement..."));

    AIController->ReceiveMoveCompleted.RemoveAll(this);
   // AIController->ReceiveMoveCompleted.AddDynamic(this, &AHintNPCSpawner::HandleMoveCompleted);

    FAIMoveRequest MoveRequest(TargetLocation->GetComponentLocation());
    MoveRequest.SetAcceptanceRadius(DefaultTargetAcceptanceRadius);

    const FPathFollowingRequestResult RequestResult = AIController->MoveTo(MoveRequest);

    if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
    {
        ActiveMoveRequestID = RequestResult.MoveId;
        return;
    }

    AIController->ReceiveMoveCompleted.RemoveAll(this);

    if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
    {
        SpawnedNPC->Destroy();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HintNPCSpawner '%s' failed to move NPC '%s' to target. Result code: %d"), *GetName(), *SpawnedNPC->GetName(), static_cast<int32>(RequestResult.Code));
       // SpawnedNPC->Destroy();
    }

    CleanupActiveNPC();
}

//void AHintNPCSpawner::HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
//{
//    if (!ActiveNPCController.IsValid() || RequestID != ActiveMoveRequestID)
//    {
//        return;
//    }
//
//    if (Result.IsSuccess() && ActiveNPC.IsValid())
//    {
//        ActiveNPC->Destroy();
//    }
//
//    CleanupActiveNPC();
//}

void AHintNPCSpawner::CleanupActiveNPC()
{
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(MovementDelayTimerHandle);
    }

    bIsMovementDelayActive = false;

    if (AAIController* Controller = ActiveNPCController.Get())
    {
        Controller->ReceiveMoveCompleted.RemoveAll(this);
    }

    ActiveNPC.Reset();
    ActiveNPCController.Reset();
    ActiveMoveRequestID = FAIRequestID::InvalidRequest;
}

void AHintNPCSpawner::RestoreWorldState()
{
    bIsWorldOverrideActive = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(ShadowWorldTimerHandle);

    if (AWorldManager* WorldManager = AWorldManager::Get(World))
    {
        WorldManager->SetWorld(PreviousWorldState);
    }
}
