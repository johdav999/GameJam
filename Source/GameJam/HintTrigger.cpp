#include "HintTrigger.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameJamGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AHintTrigger::AHintTrigger()
    : bIsPersistent(false)
    , TemporalState(EHintTemporalState::Future)
    , LoopToUnlock(0)
    , TriggerSound(nullptr)
    , bAllowRetrigger(false)
    , bTriggered(false)
    , CurrentDialogIndex(0)
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
    TriggerBox->SetGenerateOverlapEvents(true);
    RootComponent = TriggerBox;

    TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AHintTrigger::HandleOverlap);
}

void AHintTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (!TriggerBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' is missing a trigger box component."), *GetName());
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

    UE_LOG(LogTemp, Log, TEXT("Hint Triggered: %s (Added: %s)"), *HintID.ToString(), bHintAdded ? TEXT("true") : TEXT("false"));

    if (!bHintAdded && !bAllowRetrigger)
    {
        bTriggered = true;
        return;
    }

    if (TriggerSound)
    {
        UGameplayStatics::PlaySound2D(this, TriggerSound);
    }

    BeginDialogPlayback();

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

void AHintTrigger::BeginDialogPlayback()
{
    StopDialogPlayback();

    if (DialogAudio.Num() == 0)
    {
        return;
    }

    CurrentDialogIndex = 0;
    PlayNextDialogEntry();
}

void AHintTrigger::StopDialogPlayback()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DialogPlaybackHandle);
    }
}

void AHintTrigger::PlayNextDialogEntry()
{
    if (!GetWorld())
    {
        return;
    }

    UWorld* World = GetWorld();

    while (DialogAudio.IsValidIndex(CurrentDialogIndex))
    {
        const FString& AssetPath = DialogAudio[CurrentDialogIndex];
        ++CurrentDialogIndex;

        if (AssetPath.IsEmpty())
        {
            continue;
        }

        USoundBase* DialogSound = LoadObject<USoundBase>(nullptr, *AssetPath);
        if (!DialogSound)
        {
            UE_LOG(LogTemp, Warning, TEXT("HintTrigger '%s' failed to load dialog audio '%s'."), *GetName(), *AssetPath);
            continue;
        }

        UGameplayStatics::PlaySound2D(this, DialogSound);

        const float Duration = FMath::Max(DialogSound->GetDuration(), 0.1f);

        if (DialogAudio.IsValidIndex(CurrentDialogIndex))
        {
            World->GetTimerManager().SetTimer(DialogPlaybackHandle, this, &AHintTrigger::PlayNextDialogEntry, Duration, false);
        }

        return;
    }

    StopDialogPlayback();
}
