// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterPawnPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "../../../Actors/Cameras/ThirdPersonFollowCamera.h"
#include "../../../Actors/Pawns/CharacterPawn.h"
#include "../../../ActorComponents/MovementComponents/CharacterPawnMovementComponent.h"

void ACharacterPawnPlayerController::BeginPlay()
{
	Super::BeginPlay();

	AddInputMappingContext(InputMappingContext, InputMappingContextPriority);

	// Cache world pointer.
	World = GetWorld();

	// Cache controlled pawn pointer.
	CharacterPawn = Cast<ACharacterPawn>(GetPawn());
	check(IsValid(CharacterPawn));

	// Cache controlled pawn movement component.
	CharacterPawnMovement = CharacterPawn->GetCharacterPawnMovementComponent();
}

void ACharacterPawnPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//UE_LOG(LogTemp, Warning, TEXT("Controller tick"));
}

void ACharacterPawnPlayerController::BindEnhancedInputActions(UEnhancedInputComponent* const EnhancedInputComponent)
{
	EnhancedInputComponent->BindAction(LookAnalogInputAction.LoadSynchronous(), ETriggerEvent::Triggered, this, &ACharacterPawnPlayerController::OnLookAnalogInputTriggered);
	EnhancedInputComponent->BindAction(LookAbsoluteInputAction.LoadSynchronous(), ETriggerEvent::Triggered, this, &ACharacterPawnPlayerController::OnLookAbsoluteInputTriggered);

	UInputAction* LoadedMoveInputAction = MoveInputAction.LoadSynchronous();
	EnhancedInputComponent->BindAction(LoadedMoveInputAction, ETriggerEvent::Started, this, &ACharacterPawnPlayerController::OnMoveInputStarted);
	EnhancedInputComponent->BindAction(LoadedMoveInputAction, ETriggerEvent::Triggered, this, &ACharacterPawnPlayerController::OnMoveInputTriggered);
	EnhancedInputComponent->BindAction(LoadedMoveInputAction, ETriggerEvent::Completed, this, &ACharacterPawnPlayerController::OnMoveInputCompleted);

	EnhancedInputComponent->BindAction(JumpInputAction.LoadSynchronous(), ETriggerEvent::Started, this, &ACharacterPawnPlayerController::OnJumpInputStarted);
}

void ACharacterPawnPlayerController::OnLookAnalogInputTriggered(const FInputActionValue& Value)
{
	static constexpr float BaseAnalogRate = 100.0f;
	const float BaseRate = World->GetDeltaSeconds() * BaseAnalogRate;
	AddYawInput(Value[0] * BaseRate * LookAnalogYawScale);
	AddPitchInput(Value[1] * ((InvertLookPitch) ? -1.0f : 1.0f) * BaseRate * LookAnalogPitchScale);
}

void ACharacterPawnPlayerController::OnLookAbsoluteInputTriggered(const FInputActionValue& Value)
{
	AddYawInput(Value[0]);
	AddPitchInput(Value[1] * ((InvertLookPitch) ? 1.0f : -1.0f));
}

void ACharacterPawnPlayerController::OnMoveInputStarted(const FInputActionValue& Value)
{
}

void ACharacterPawnPlayerController::OnMoveInputTriggered(const FInputActionValue& Value)
{
	// Calculate movement direction and magnitude.
	const FRotator ControlYawRotation(FRotator(0.0, GetControlRotation().Yaw, 0.0));
	const FVector MovementInputDirection = ControlYawRotation.RotateVector(FVector(Value[1], Value[0], 0.0)).GetSafeNormal();
	const float MovementInputMagnitude = FMath::Clamp(Value.GetMagnitude(), 0.f, 1.f);

	// Move the pawn.
	CharacterPawnMovement->AddMovementInput(MovementInputDirection, MovementInputMagnitude);
}

void ACharacterPawnPlayerController::OnMoveInputCompleted(const FInputActionValue& Value)
{
}

void ACharacterPawnPlayerController::OnJumpInputStarted(const FInputActionValue& Value)
{
	CharacterPawnMovement->Jump();
}
