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

AGameJamCharacter::AGameJamCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
        // Don't rotate when the controller rotates. Let that just affect the camera.
        bUseControllerRotationPitch = false;
        bUseControllerRotationYaw = false;
        bUseControllerRotationRoll = false;

        // Configure character movement for third-person traversal
        GetCharacterMovement()->bOrientRotationToMovement = true;
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

        // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
        // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AGameJamCharacter::BeginPlay()
{
        Super::BeginPlay();

        if (!WorldShiftEffects)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* Manager = AWorldManager::Get(World))
                {
                        Manager->OnWorldShifted.AddDynamic(WorldShiftEffects, &UWorldShiftEffectsComponent::TriggerWorldShiftEffects);
                }
        }
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
        const float AxisValue = Value.Get<float>();
        if (FMath::IsNearlyZero(AxisValue))
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                if (AWorldManager* WorldManager = AWorldManager::Get(World))
                {
                        if (AxisValue > 0.0f)
                        {
                                WorldManager->ShiftToNextWorld();
                        }
                        else
                        {
                                WorldManager->ShiftToPreviousWorld();
                        }
                }
        }
}

void AGameJamCharacter::DoMove(float Right, float Forward)
{


        if (Controller != nullptr)
        {
                const FRotator ControlRotation = Controller->GetControlRotation();
                const FRotator YawRotation(0, ControlRotation.Yaw, 0);

                const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
                const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

                AddMovementInput(RightDirection, Right);
                AddMovementInput(ForwardDirection, Forward);
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
