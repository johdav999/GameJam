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
class UTimeShiftEffortComponent;
class UDialogAudioComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractInputSignature);

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

        /** Tracks the effort required to sustain Chaos world shifts. */
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TimeShift", meta = (AllowPrivateAccess = "true"))
        UTimeShiftEffortComponent* TimeShiftEffortComponent;

        /** Handles spatialized playback for character dialogue. */
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
        UDialogAudioComponent* DialogAudioComponent;
	
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

        /** Shift World Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* ShiftWorldAction;

        /** Interact Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* InteractAction;

public:

        /** Constructor */
        AGameJamCharacter();

        /** Starts the automatic world introduction sequence. */
        UFUNCTION(BlueprintCallable, Category="World Shift|Intro")
        void StartIntroWorldSequence();

        /** Broadcast whenever the interact input is triggered. */
        UPROPERTY(BlueprintAssignable, Category="Input|Interact")
        FOnInteractInputSignature OnInteract;

        bool ShouldNotResetWorld = false;

protected:

        virtual void BeginPlay() override;

        void Tick(float DeltaSeconds) override;

        virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

        /** Initialize input action bindings */
        virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

        virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

        /** Called for movement input */
        void Move(const FInputActionValue& Value);

        /** Called for looking input */
        void Look(const FInputActionValue& Value);

        /** Handles entering the Chaos world when the shift input is pressed. */
        void OnShiftPressed();

        /** Handles returning to the Light world when the shift input is released. */
        void OnShiftReleased();

        /** Event fired when a manual time shift into Chaos begins. */
        UFUNCTION(BlueprintImplementableEvent, Category="World Shift|Input")
        void OnTimeShiftStarted();

        /** Called for interact input. */
        void Interact(const FInputActionValue& Value);

        /** Synchronizes local state whenever the active world changes. */
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

        /** Handles interact inputs from either controls or UI interfaces */
        UFUNCTION(BlueprintCallable, Category="Input")
        virtual void DoInteract();


public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

        /** Returns FollowCamera subobject **/
        FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

        /** Returns TimeShiftEffortComponent subobject **/
        FORCEINLINE class UTimeShiftEffortComponent* GetTimeShiftEffortComponent() const { return TimeShiftEffortComponent; }

        /** Returns DialogAudioComponent subobject **/
        FORCEINLINE class UDialogAudioComponent* GetDialogAudioComponent() const { return DialogAudioComponent; }

private:
        /** Tracks whether manual world shifting is currently allowed. */
        bool bManualWorldShiftEnabled = true;

        /** Tracks whether the player currently has the Chaos world shift held. */
        bool bChaosShiftActive = false;

        /** Tracks whether the intro sequence is currently running. */
        bool bIntroSequenceActive = false;

        /** Walk speed applied while the intro sequence is active. */
        UPROPERTY(EditAnywhere, Category="World Shift|Intro", meta = (ClampMin = "0.0"))
        float IntroWalkSpeed = 150.0f;

        /** Cached walk speed restored once the intro sequence ends. */
        float CachedWalkSpeed = 0.0f;

        /** Cached jump Z velocity restored once the intro sequence ends. */
        float CachedJumpZVelocity = 0.0f;

        /** Timer used to transition from Chaos back to Light during the intro sequence. */
        FTimerHandle IntroLightTimerHandle;

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


        /** Starts the delayed world reset process. */
        void StartFallingResetTimer();

        /** Cancels the delayed world reset when the character recovers. */
        void CancelFallingResetTimer();

        /** Executes when the falling reset delay expires. */
        void HandleFallingResetTimerElapsed();
};

