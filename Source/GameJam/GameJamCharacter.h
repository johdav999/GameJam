// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "WorldShiftTypes.h"
#include "GameJamCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UWorldShiftEffectsComponent;
class UHealthComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AGameJamCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

        /** Follow camera */
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
        UCameraComponent* FollowCamera;

        /** Handles audiovisual feedback when the world state changes */
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="World Shift", meta = (AllowPrivateAccess = "true"))
        UWorldShiftEffectsComponent* WorldShiftEffects;

        /** Handles player health state and broadcasts updates */
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
        UHealthComponent* HealthComponent;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

        /** Look Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* LookAction;

        /** Mouse Look Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* MouseLookAction;

        /** Cycle World Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* CycleWorldAction;

public:

        /** Constructor */
        AGameJamCharacter();

        /** Starts the automatic world introduction sequence. */
        UFUNCTION(BlueprintCallable, Category="World Shift|Intro")
        void StartIntroWorldSequence();

protected:

        virtual void BeginPlay() override;

        void Tick(float DeltaSeconds) override;

        /** Initialize input action bindings */
        virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

        virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

        /** Called for movement input */
        void Move(const FInputActionValue& Value);

        /** Called for looking input */
        void Look(const FInputActionValue& Value);

        /** Cycles through the available world states. */
        void CycleWorld(const FInputActionValue& Value);

        /** Applies the health penalty whenever the active world changes. */
        UFUNCTION()
        void HandleWorldShifted(EWorldState NewWorld);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();


public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

        /** Returns FollowCamera subobject **/
        FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

        /** Returns HealthComponent subobject **/
        FORCEINLINE class UHealthComponent* GetHealthComponent() const { return HealthComponent; }

private:
        /** Tracks whether the initial world state notification has been received. */
        bool bReceivedInitialWorldNotification = false;

        /** Tracks whether manual world shifting is currently allowed. */
        bool bManualWorldShiftEnabled = true;

        /** Tracks whether the intro sequence is currently running. */
        bool bIntroSequenceActive = false;

        /** Walk speed applied while the intro sequence is active. */
        UPROPERTY(EditAnywhere, Category="World Shift|Intro", meta = (ClampMin = "0.0"))
        float IntroWalkSpeed = 150.0f;

        /** Cached walk speed restored once the intro sequence ends. */
        float CachedWalkSpeed = 0.0f;

        /** Cached jump Z velocity restored once the intro sequence ends. */
        float CachedJumpZVelocity = 0.0f;

        /** Timer used to transition from Chaos to Dream. */
        FTimerHandle IntroDreamTimerHandle;

        /** Timer used to transition from Dream to Light. */
        FTimerHandle IntroLightTimerHandle;

        /** Applies the Dream world during the intro sequence. */
        void HandleIntroDreamTransition();

        /** Applies the Light world and restores control when the intro ends. */
        void HandleIntroLightTransition();

        /** Restores player control after the intro sequence. */
        void RestoreControlAfterIntro();

        /** Delay applied before triggering a world reset after falling begins. */
        UPROPERTY(EditAnywhere, Category="World Shift|Falling", meta = (ClampMin = "0.0"))
        float FallingResetDelay = 1.0f;

        /** Tracks whether a reset has already been queued while falling. */
        bool bFallingResetTimerActive = false;

        /** Timer used to delay the world reset after a fall is detected. */
        FTimerHandle FallingResetTimerHandle;

        /** Handles movement mode changes to detect the start/end of falls. */
        void HandleMovementModeChanged(ACharacter* InCharacter, EMovementMode PreviousMovementMode, uint8 PreviousCustomMode);

        /** Starts the delayed world reset process. */
        void StartFallingResetTimer();

        /** Cancels the delayed world reset when the character recovers. */
        void CancelFallingResetTimer();

        /** Executes when the falling reset delay expires. */
        void HandleFallingResetTimerElapsed();
};

