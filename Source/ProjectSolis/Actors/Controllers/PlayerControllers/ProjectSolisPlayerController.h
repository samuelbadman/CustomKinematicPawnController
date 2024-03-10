// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../../../ProjectBaseClasses/ProjectPlayerController.h"
#include "ProjectSolisPlayerController.generated.h"

struct FInputActionValue;
class UInputMappingContext;
class UInputAction;

/**
 *
 */
UCLASS()
class PROJECTSOLIS_API AProjectSolisPlayerController : public AProjectPlayerController
{
	GENERATED_BODY()

protected:
	// Adds a mapping context to the local player enhanced input subsystem with the specified priority.
	void AddInputMappingContext(TSoftObjectPtr<UInputMappingContext> SoftInputMappingContext, int32 Priority);

private:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	virtual void BindEnhancedInputActions(UEnhancedInputComponent* const EnhancedInputComponent);

	void BindInputActions();
	void BindAnyKeyInput();

	void OnMouseMove();

	UFUNCTION()
	void OnAnyKeyPressed(const FKey Key);

	UFUNCTION()
	void OnAnyKeyReleased(const FKey Key);

	// Returns true if the mouse position has changed since the last time this function was called.
	bool DetectMouseMove();
};
