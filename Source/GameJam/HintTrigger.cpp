#include "HintTrigger.h"

#include "Components/BoxComponent.h"
#include "DialogAudioComponent.h"
#include "GameFramework/Character.h"
#include "GameJamGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"

AHintTrigger::AHintTrigger()
    : bIsPersistent(false)
    , TemporalState(EHintTemporalState::Future)
    , LoopToUnlock(0)
    , TriggerSound(nullptr)
    , bAllowRetrigger(false)
    , bTriggered(false)
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
    TriggerBox->SetGenerateOverlapEvents(true);
    RootComponent = TriggerBox;

    DialogAudioComponent = CreateDefaultSubobject<UDialogAudioComponent>(TEXT("DialogAudioComponent"));
    DialogAudioComponent->SetupAttachment(RootComponent);

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AHintTrigger::HandleOverlap);
}

void AHintTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (!TriggerBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' is missing a trigger box component."), *GetName());
    }

    if (!DialogAudioComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' missing DialogAudioComponent."), *GetName());
    }
}

void AHintTrigger::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor)
    {
        return;
    }

    if (bTriggered && !bAllowRetrigger)
    {
        return;
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor != PlayerCharacter)
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UGameJamGameInstance* GameJamGameInstance = Cast<UGameJamGameInstance>(GameInstance);
    if (!GameJamGameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' could not access UGameJamGameInstance."), *GetName());
        return;
    }

    const bool bHintAdded = GameJamGameInstance->AddHint(HintID, HintText, bIsPersistent, TemporalState, LoopToUnlock, DialogAudio);

    if (!bHintAdded && !bAllowRetrigger)
    {
        bTriggered = true;
        return;
    }

    if (TriggerSound)
    {
        UGameplayStatics::PlaySound2D(this, TriggerSound);
    }

    if (DialogAudioComponent && DialogAudio.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Hint '%s' triggered. Starting dialog playback (%d lines)."), *HintID.ToString(), DialogAudio.Num());

        TArray<TSoftObjectPtr<USoundBase>> SoftLines;
        SoftLines.Reserve(DialogAudio.Num());

        for (const FString& Path : DialogAudio)
        {
            if (!Path.IsEmpty())
            {
                SoftLines.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(Path)));
            }
        }

        if (SoftLines.Num() > 0)
        {
            DialogAudioComponent->QueueDialogue(SoftLines, 0.2f);
        }
    }
    else if (!DialogAudioComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' missing DialogAudioComponent."), *GetName());
    }

    OnHintTriggered();

    if (!bAllowRetrigger)
    {
        bTriggered = true;
    }
    else
    {
        bTriggered = false;
    }
}
