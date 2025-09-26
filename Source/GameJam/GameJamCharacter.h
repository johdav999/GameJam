// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Variant_SideScrolling/Gameplay/PaintZone.h"
#include "GameJamCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class APaintZone;

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

        /** Paint Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* PaintAction;

        /** Switch Paint Type Input Action */
        UPROPERTY(EditAnywhere, Category="Input")
        UInputAction* SwitchPaintTypeAction;

public:

        /** Constructor */
        AGameJamCharacter();

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

        /** Called for movement input */
        void Move(const FInputActionValue& Value);

        /** Called for looking input */
        void Look(const FInputActionValue& Value);

        /** Fires a paint projectile that creates a paint zone. */
        void ShootPaint();

        /** Cycles through the available paint force types. */
        void CyclePaintType(const FInputActionValue& Value);

        UPROPERTY(EditDefaultsOnly, Category="Paint")
        TSubclassOf<class APaintZone> PaintZoneClass;

        UPROPERTY(EditDefaultsOnly, Category="Paint")
        float PaintRange = 2000.f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Paint")
        EForceType CurrentForceType = EForceType::Push;

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
};

