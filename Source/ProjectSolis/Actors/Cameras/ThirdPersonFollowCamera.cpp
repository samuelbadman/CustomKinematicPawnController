// Fill out your copyright notice in the Description page of Project Settings.

#include "ThirdPersonFollowCamera.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Pawns/CharacterPawn.h"

AThirdPersonFollowCamera::AThirdPersonFollowCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(FName(TEXT("SceneRootComponent")));
	RootComponent = SceneRoot;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("CameraComponent")));
	Camera->SetupAttachment(RootComponent);
}

void AThirdPersonFollowCamera::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	PlayerController = UGameplayStatics::GetPlayerController(World, 0);

	// Ensure this actor ticks after the player controller and the player pawn.
	// This ensures the player controller's rotation is updated from look input and the player pawn is moved before the camera updates its viewpoint.
	AddTickPrerequisiteActor(PlayerController);
	AddTickPrerequisiteActor(PlayerPawn);

	ApplyRelativeBackOffsetValue(RelativeBackOffset);

	// Look through this camera.
	PlayerController->SetViewTarget(this);
}

void AThirdPersonFollowCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UE_LOG(LogTemp, Warning, TEXT("Camera tick"));

	// Rotate actor to match control rotation.
	SetActorRotation(PlayerController->GetControlRotation());
	// Move the camera to the player pawn.
	SetActorLocation(PlayerPawn->GetActorLocation());

	UpdateRelativeBackOffsetValue();
}

void AThirdPersonFollowCamera::ApplyRelativeBackOffsetValue(const float Value)
{
	FVector NewRelativeCameraLocation = Camera->GetRelativeLocation();
	NewRelativeCameraLocation.X = -static_cast<double>(Value);
	Camera->SetRelativeLocation(NewRelativeCameraLocation);
}

void AThirdPersonFollowCamera::UpdateRelativeBackOffsetValue()
{
	// Calculate new relative back offset value based on how much the camera is looking up or down.
	float NewRelativeBackOffset = RelativeBackOffset;
	const float ActorForwardDotWorldUp = -static_cast<float>(FVector::DotProduct(GetActorForwardVector(), FVector::UpVector));
	if (ActorForwardDotWorldUp > 0.f)
	{
		// Looking down.
		NewRelativeBackOffset += ScaledAdditiveRelativeBackOffsetLookDownValue * FMath::Abs(ActorForwardDotWorldUp);
	}
	else
	{
		// Looking up.
		NewRelativeBackOffset += ScaledAdditiveRelativeBackOffsetLookUpValue * FMath::Abs(ActorForwardDotWorldUp);
	}

	// Apply relative back offset to the camera components relative location overwriting any previous relative X location.
	ApplyRelativeBackOffsetValue(NewRelativeBackOffset);
}
