// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CameraActorBase.h"
#include "ThirdPersonFollowCamera.generated.h"

class UCameraComponent;

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API AThirdPersonFollowCamera : public ACameraActorBase
{
	GENERATED_BODY()
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	// Relative translation offset along the negative x axis that moves the camera back from the actor root component.
	UPROPERTY(EditDefaultsOnly, Category = "ThirdPersonFollowCamera", meta = (ClampMin = "0.0"))
	float RelativeBackOffset = 300.0f;

	// Value added to RelativeBackOffset when the camera is looking up scaled by the amount the camera is looking up.
	UPROPERTY(EditDefaultsOnly, Category = "ThirdPersonFollowCamera")
	float ScaledAdditiveRelativeBackOffsetLookUpValue = -250.f;

	// Value added to RelativeBackOffset when the camera is looking down scaled by the amount the camera is looking down.
	UPROPERTY(EditDefaultsOnly, Category = "ThirdPersonFollowCamera")
	float ScaledAdditiveRelativeBackOffsetLookDownValue = 100.f;

	UWorld* World = nullptr;
	APawn* PlayerPawn = nullptr;
	APlayerController* PlayerController = nullptr;

private:
	AThirdPersonFollowCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Applies the relative back offset value based on how much the camera is looking up or down.
	void ApplyRelativeBackOffsetValue(const float Value);

	// Updates teh relative back offset value.
	void UpdateRelativeBackOffsetValue();
};
