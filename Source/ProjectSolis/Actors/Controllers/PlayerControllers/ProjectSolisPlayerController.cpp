// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectSolisPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Commands/InputChord.h"
#include "InputMappingContext.h"

void AProjectSolisPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BindAnyKeyInput();
	BindInputActions();
}

void AProjectSolisPlayerController::BindEnhancedInputActions(UEnhancedInputComponent* const EnhancedInputComponent)
{
	// Note: Optionally implemented by child classes.
}

void AProjectSolisPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// Check for mouse position change since the last frame and call event function if the mouse has been moved.
	if (DetectMouseMove())
	{
		OnMouseMove();
	}
}

void AProjectSolisPlayerController::BindInputActions()
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		BindEnhancedInputActions(EnhancedInputComponent);
	}
}

void AProjectSolisPlayerController::BindAnyKeyInput()
{
	FInputKeyBinding AnyKeyPressedBinding(FInputChord(EKeys::AnyKey), EInputEvent::IE_Pressed);
	AnyKeyPressedBinding.bConsumeInput = false;
	AnyKeyPressedBinding.bExecuteWhenPaused = true;
	AnyKeyPressedBinding.KeyDelegate.GetDelegateWithKeyForManualSet().BindUObject(this, &AProjectSolisPlayerController::OnAnyKeyPressed);

	InputComponent->KeyBindings.Add(AnyKeyPressedBinding);

	FInputKeyBinding AnyKeyReleasedBinding(FInputChord(EKeys::AnyKey), EInputEvent::IE_Released);
	AnyKeyReleasedBinding.bConsumeInput = false;
	AnyKeyReleasedBinding.bExecuteWhenPaused = true;
	AnyKeyReleasedBinding.KeyDelegate.GetDelegateWithKeyForManualSet().BindUObject(this, &AProjectSolisPlayerController::OnAnyKeyReleased);

	InputComponent->KeyBindings.Add(AnyKeyReleasedBinding);
}

void AProjectSolisPlayerController::OnAnyKeyPressed(const FKey Key)
{
}

void AProjectSolisPlayerController::OnAnyKeyReleased(const FKey Key)
{
}

void AProjectSolisPlayerController::OnMouseMove()
{
}

void AProjectSolisPlayerController::AddInputMappingContext(TSoftObjectPtr<UInputMappingContext> SoftInputMappingContext, int32 Priority)
{
	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			check(!SoftInputMappingContext.IsNull());
			UInputMappingContext* InputMappingContext = SoftInputMappingContext.LoadSynchronous();
			if (IsValid(InputMappingContext))
			{
				InputSystem->AddMappingContext(InputMappingContext, Priority);
			}
		}
	}
}

bool AProjectSolisPlayerController::DetectMouseMove()
{
	static FVector2D PreviousMousePosition = FVector2D(0.0f);

	FVector2D CurrentMousePosition = FVector2D::ZeroVector;

	// GetMousePosition returns false if there is no associated mouse device.
	if (!GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y))
	{
		return false;
	}

	if (CurrentMousePosition != PreviousMousePosition)
	{
		// Mouse position has changed.
		PreviousMousePosition = CurrentMousePosition;
		return true;
	}

	// Mouse position has not changed.
	return false;
}
