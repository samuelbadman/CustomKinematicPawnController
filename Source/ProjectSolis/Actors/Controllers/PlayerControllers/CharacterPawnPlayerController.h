// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSolisPlayerController.h"
#include "CharacterPawnPlayerController.generated.h"

class UEnhancedInputComponent;
class AThirdPersonFollowCamera;
class ACharacterPawn;
class UCharacterPawnMovementComponent;

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API ACharacterPawnPlayerController : public AProjectSolisPlayerController
{
	GENERATED_BODY()

private:
	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	TSoftObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	int32 InputMappingContextPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	TSoftObjectPtr<UInputAction> LookAnalogInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	TSoftObjectPtr<UInputAction> LookAbsoluteInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	TSoftObjectPtr<UInputAction> MoveInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Input")
	TSoftObjectPtr<UInputAction> JumpInputAction = nullptr;

	// Analog horizontal sensitivity.
	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Camera")
	float LookAnalogYawScale = 1.0f;

	// Analog vertical sensitivity.
	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Camera")
	float LookAnalogPitchScale = 1.0f;

	// Inverts analog and absolute pitch input.
	UPROPERTY(EditDefaultsOnly, Category = "CharacterPawnPlayerController|Camera")
	bool InvertLookPitch = false;

	// World.
	UWorld* World = nullptr;

	// Controlled character pawn.
	ACharacterPawn* CharacterPawn = nullptr;

	// Controlled character pawn movement component.
	UCharacterPawnMovementComponent* CharacterPawnMovement = nullptr;

private:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BindEnhancedInputActions(UEnhancedInputComponent* const EnhancedInputComponent) override;

	void OnLookAnalogInputTriggered(const FInputActionValue& Value);
	void OnLookAbsoluteInputTriggered(const FInputActionValue& Value);
	void OnMoveInputStarted(const FInputActionValue& Value);
	void OnMoveInputTriggered(const FInputActionValue& Value);
	void OnMoveInputCompleted(const FInputActionValue& Value);
	void OnJumpInputStarted(const FInputActionValue& Value);
};
