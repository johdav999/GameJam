// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
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

protected:

        virtual void BeginPlay() override;

        /** Initialize input action bindings */
        virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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
};

