// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterPawn.h"
#include "../../ActorComponents/MovementComponents/CharacterPawnMovementComponent.h"

ACharacterPawn::ACharacterPawn()
{
	// Create and setup kinematic pawn movement component.
	CharacterPawnMovement = CreateDefaultSubobject<UCharacterPawnMovementComponent>(FName(TEXT("CharacterPawnMovementComponent")));
}

FCollisionShape ACharacterPawn::GetMovementCollisionShape() const
{
	unimplemented();
	return FCollisionShape();
}

FVector ACharacterPawn::GetMovementCollisionLocation() const
{
	unimplemented();
	return FVector();
}

FQuat ACharacterPawn::GetMovementCollisionRotation() const
{
	unimplemented();
	return FQuat();
}
