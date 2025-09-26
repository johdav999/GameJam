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
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Gameplay/PaintZone.h"
#include "GameJam.h"

AGameJamCharacter::AGameJamCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
        // Don't rotate when the controller rotates. Let that just affect the camera.
        bUseControllerRotationPitch = false;
        bUseControllerRotationYaw = false;
        bUseControllerRotationRoll = false;

        // Configure character movement for a side-scroller
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        GetCharacterMovement()->SetPlaneConstraintEnabled(true);
        GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.f, 1.f, 0.f));
        GetCharacterMovement()->bConstrainToPlane = true;

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
        CameraBoom->TargetArmLength = 600.0f;
        CameraBoom->bUsePawnControlRotation = false;
        CameraBoom->SetUsingAbsoluteRotation(true);
        CameraBoom->bDoCollisionTest = false;
        CameraBoom->SetWorldRotation(FRotator(0.f, -90.f, 0.f));

	// Create a follow camera
        FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
        FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
        FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
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

                // Painting
                EnhancedInputComponent->BindAction(PaintAction, ETriggerEvent::Started, this, &AGameJamCharacter::ShootPaint);

                // Switching paint type
                EnhancedInputComponent->BindAction(SwitchPaintTypeAction, ETriggerEvent::Triggered, this, &AGameJamCharacter::CyclePaintType);
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

        // route the input (X mapped to horizontal movement)
        DoMove(MovementVector.X, 0.0f);
}

void AGameJamCharacter::Look(const FInputActionValue& Value)
{
        // input is a Vector2D
        FVector2D LookAxisVector = Value.Get<FVector2D>();

        // route the input
        DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGameJamCharacter::ShootPaint()
{
        if (!PaintZoneClass)
        {
                return;
        }

        UWorld* World = GetWorld();
        if (!World)
        {
                return;
        }

        const FVector TraceStart = FollowCamera ? FollowCamera->GetComponentLocation() : GetActorLocation();
        const FVector TraceDirection = FollowCamera ? FollowCamera->GetForwardVector() : GetActorForwardVector();
        const FVector TraceEnd = TraceStart + TraceDirection * PaintRange;

        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShootPaint), true, this);
        QueryParams.AddIgnoredActor(this);

        if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
                return;
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = GetInstigator();

        APaintZone* Zone = World->SpawnActor<APaintZone>(PaintZoneClass, Hit.Location, FRotator::ZeroRotator, SpawnParams);
        if (Zone)
        {
                Zone->InitializeFromHit(Hit, CurrentForceType);
        }
}

void AGameJamCharacter::CyclePaintType(const FInputActionValue& Value)
{
        const float AxisValue = Value.Get<float>();
        if (FMath::IsNearlyZero(AxisValue))
        {
                return;
        }

        constexpr int32 ForceTypeCount = 3;
        const int32 Direction = AxisValue > 0.0f ? 1 : -1;
        int32 ForceTypeIndex = static_cast<int32>(CurrentForceType);
        ForceTypeIndex = (ForceTypeIndex + Direction + ForceTypeCount) % ForceTypeCount;
        CurrentForceType = static_cast<EForceType>(ForceTypeIndex);
}

void AGameJamCharacter::DoMove(float Right, float Forward)
{


        if (GetController() != nullptr)
        {
                // Move strictly along the world X axis for side-scrolling
                const FVector SideScrollDirection = FVector(1.f, 0.f, 0.f);
                AddMovementInput(SideScrollDirection, Right);
        }
}

void AGameJamCharacter::DoLook(float Yaw, float Pitch)
{

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
