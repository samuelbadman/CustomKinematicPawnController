// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "../../Libraries/MathUtilityLibrary.h"
#include "../../ActorComponents/MovementComponents/CharacterPawnMovementComponent.h"

APlayerPawn::APlayerPawn()
{
	// Create and setup capsule component.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(FName(TEXT("CapsuleCollision")));
	RootComponent = Capsule;
	Capsule->SetCapsuleHalfHeight(92.f, false);
	Capsule->SetCapsuleRadius(40.f, false);

	GetCharacterPawnMovementComponent()->SetUpdatedComponent(Capsule);

	// Create and setup arrow component.
	Arrow = CreateDefaultSubobject<UArrowComponent>(FName(TEXT("DebugArrow")));
	Arrow->SetupAttachment(RootComponent);
	Arrow->SetArrowColor(FColor::FromHex(FString(TEXT("9FBCFFFF"))));

	// Create and setup skeletal mesh component.
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName(TEXT("SkeletalMesh")));
	Mesh->SetupAttachment(RootComponent);

	GetCharacterPawnMovementComponent()->SetRootMotionMesh(Mesh);
}

void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UE_LOG(LogTemp, Warning, TEXT("Pawn tick"));
}

FCollisionShape APlayerPawn::GetMovementCollisionShape() const
{
	return FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
}

FVector APlayerPawn::GetMovementCollisionLocation() const
{
	return Capsule->GetComponentLocation();
}

FQuat APlayerPawn::GetMovementCollisionRotation() const
{
	return Capsule->GetComponentQuat();
}
