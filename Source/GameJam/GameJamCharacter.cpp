// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameJamCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "Math/RotationMatrix.h"
#include "Math/UnrealMathUtility.h"
#include "GameJam.h"
#include "WorldManager.h"
#include "WorldShiftEffectsComponent.h"
#include "HealthComponent.h"
#include "TimerManager.h"

AGameJamCharacter::AGameJamCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
        // Don't rotate when the controller rotates. Let that just affect the camera.



        bUseControllerRotationPitch = false;
        bUseControllerRotationYaw =true;
        bUseControllerRotationRoll = false;


        CameraBoom->bUsePawnControlRotation = true;

// The camera itself should NOT rotate relative to the boom


// Let controller rotation affect the character yaw



        // Configure character movement for third-person traversal
        GetCharacterMovement()->bOrientRotationToMovement = false;
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        GetCharacterMovement()->SetPlaneConstraintEnabled(false);
        GetCharacterMovement()->bConstrainToPlane = false;

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
        CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
        CameraBoom->SetupAttachment(RootComponent);
        CameraBoom->TargetArmLength = 300.0f;
        CameraBoom->bUsePawnControlRotation = true;
        CameraBoom->bDoCollisionTest = true;

	// Create a follow camera
        FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
        FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
        FollowCamera->bUsePawnControlRotation = false;

        // Create the world shift effects component responsible for audiovisual feedback
        WorldShiftEffects = CreateDefaultSubobject<UWorldShiftEffectsComponent>(TEXT("WorldShiftEffects"));

        // Create the health component responsible for managing player health
        HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

        // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
        // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AGameJamCharacter::StartIntroWorldSequence()
{
        if (bIntroSequenceActive)
        {
                return;
        }

        bIntroSequenceActive = true;
        bManualWorldShiftEnabled = false;

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
                CachedWalkSpeed = Movement->MaxWalkSpeed;
                CachedJumpZVelocity = Movement->JumpZVelocity;

                if (IntroWalkSpeed > 0.0f)
                {
                        Movement->MaxWalkSpeed = IntroWalkSpeed;
                }

                Movement->JumpZVelocity = 0.0f;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        Manager->SetWorld(EWorldState::Chaos);
                }

                FTimerManager& TimerManager = World->GetTimerManager();
                TimerManager.SetTimer(IntroDreamTimerHandle, this, &AGameJamCharacter::HandleIntroDreamTransition, 5.0f, false);
                TimerManager.SetTimer(IntroLightTimerHandle, this, &AGameJamCharacter::HandleIntroLightTransition, 10.0f, false);
        }
}

void AGameJamCharacter::BeginPlay()
{
        Super::BeginPlay();

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        if (WorldShiftEffects)
                        {
                                Manager->OnWorldShifted.AddDynamic(WorldShiftEffects, &UWorldShiftEffectsComponent::TriggerWorldShiftEffects);
                        }

                        Manager->OnWorldShifted.AddDynamic(this, &AGameJamCharacter::HandleWorldShifted);
                }
        }
}

void AGameJamCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
        CancelFallingResetTimer();

        Super::EndPlay(EndPlayReason);
}

void AGameJamCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
                // Jumping
                EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
                EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

                // Moving
                EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGameJamCharacter::Move);
                EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGameJamCharacter::Look);

                // Looking
                EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGameJamCharacter::Look);

                // World shifting
                EnhancedInputComponent->BindAction(CycleWorldAction, ETriggerEvent::Triggered, this, &AGameJamCharacter::CycleWorld);
        }
        else
        {
                UE_LOG(LogGameJam, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
        }
}

void AGameJamCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
        FVector2D MovementVector = Value.Get<FVector2D>();

        // route the input to the movement handler (X is right, Y is forward)
        DoMove(MovementVector.X, MovementVector.Y);
}

void AGameJamCharacter::Look(const FInputActionValue& Value)
{
        // input is a Vector2D
        FVector2D LookAxisVector = Value.Get<FVector2D>();

        // route the input
        DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGameJamCharacter::CycleWorld(const FInputActionValue& Value)
{
        if (!bManualWorldShiftEnabled)
        {
                return;
        }

        const float AxisValue = Value.Get<float>();
        if (FMath::IsNearlyZero(AxisValue))
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* WorldManager = AWorldManager::Get(World))
                {
                        const int32 NumWorlds = static_cast<int32>(EWorldState::Chaos) + 1;
                        int32 CurrentIndex = static_cast<int32>(WorldManager->GetCurrentWorld());

                        if (AxisValue > 0.0f)
                        {
                                CurrentIndex = (CurrentIndex + 1) % NumWorlds;
                        }
                        else if (AxisValue < 0.0f)
                        {
                                CurrentIndex = (CurrentIndex - 1 + NumWorlds) % NumWorlds;
                        }

                        const EWorldState NewWorld = static_cast<EWorldState>(CurrentIndex);
                        WorldManager->SetWorld(NewWorld);
                }
        }
}

void AGameJamCharacter::HandleWorldShifted(EWorldState NewWorld)
{
        if (!bReceivedInitialWorldNotification)
        {
                bReceivedInitialWorldNotification = true;
                return;
        }

        if (HealthComponent)
        {
                constexpr float WorldShiftPenalty = 10.0f;
                HealthComponent->ApplyDamage(WorldShiftPenalty);
        }
}

void AGameJamCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
        Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
                const bool bNowFalling = Movement->IsFalling();

                if (bNowFalling)
                {
                        if (PrevMovementMode != MOVE_Falling && Movement->Velocity.Z <= 0.0f)
                        {
                                StartFallingResetTimer();
                        }
                }
                else
                {
                        CancelFallingResetTimer();
                }
        }
}

void AGameJamCharacter::StartFallingResetTimer()
{
        if (bFallingResetTimerActive)
        {
                return;
        }

        if (FallingResetDelay <= 0.0f)
        {
                HandleFallingResetTimerElapsed();
                return;
        }

        if (UWorld* World = GetWorld())
        {
                bFallingResetTimerActive = true;
                World->GetTimerManager().SetTimer(FallingResetTimerHandle, this, &AGameJamCharacter::HandleFallingResetTimerElapsed, FallingResetDelay, false);
        }
}

void AGameJamCharacter::CancelFallingResetTimer()
{
        if (!bFallingResetTimerActive)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                World->GetTimerManager().ClearTimer(FallingResetTimerHandle);
        }

        bFallingResetTimerActive = false;
}

void AGameJamCharacter::HandleFallingResetTimerElapsed()
{
        bFallingResetTimerActive = false;

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
                if (!Movement->IsFalling())
                {
                        return;
                }
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        Manager->ResetWorld();
                }
        }
}

void AGameJamCharacter::HandleIntroDreamTransition()
{
        if (!bIntroSequenceActive)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        // Treat the Shadow state as the Dream world for the intro sequence.
                        Manager->SetWorld(EWorldState::Shadow);
                }
        }
}

void AGameJamCharacter::HandleIntroLightTransition()
{
        if (!bIntroSequenceActive)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        Manager->SetWorld(EWorldState::Light);
                }
        }

        RestoreControlAfterIntro();
}

void AGameJamCharacter::RestoreControlAfterIntro()
{
        bIntroSequenceActive = false;
        bManualWorldShiftEnabled = true;

        if (UWorld* World = GetWorld())
        {
                FTimerManager& TimerManager = World->GetTimerManager();
                TimerManager.ClearTimer(IntroDreamTimerHandle);
                TimerManager.ClearTimer(IntroLightTimerHandle);
        }

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
                Movement->MaxWalkSpeed = CachedWalkSpeed;
                Movement->JumpZVelocity = CachedJumpZVelocity;
        }
}

void AGameJamCharacter::DoMove(float Right, float Forward)
{


        if (Controller != nullptr)
        {
                const FRotator ControlRotation = Controller->GetControlRotation();
                const FRotator YawRotation(0, ControlRotation.Yaw, 0);
                UE_LOG(LogTemp, Warning, TEXT("YawRotation: Pitch=%f  Yaw=%f  Roll=%f"),
                    YawRotation.Pitch, YawRotation.Yaw, YawRotation.Roll);
              

                UE_LOG(LogTemp, Warning, TEXT("ControlRotation: Pitch=%f  Yaw=%f  Roll=%f"),
                    ControlRotation.Pitch, ControlRotation.Yaw, ControlRotation.Roll);
                const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
                const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
                const FVector MoveDirection = (ForwardDirection * Forward + RightDirection * Right).GetSafeNormal();

                AddMovementInput(RightDirection, Right);
                AddMovementInput(ForwardDirection, Forward);
                // Only rotate if moving
                if (!MoveDirection.IsNearlyZero())
                {
                    FRotator TargetRotation = MoveDirection.Rotation();
                    SetActorRotation(TargetRotation);
                }
        }
}

void AGameJamCharacter::DoLook(float Yaw, float Pitch)
{
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
}

void AGameJamCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AGameJamCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
void AGameJamCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (Controller)
    {
        FRotator ControlRot = Controller->GetControlRotation();
        FRotator TargetRot(0.f, ControlRot.Yaw, 0.f);

        // Smoothly interpolate
        FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, 5.f);
        SetActorRotation(NewRot);
    }
}