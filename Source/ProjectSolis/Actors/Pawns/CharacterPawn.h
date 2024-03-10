// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSolisPawn.h"
#include "CharacterPawn.generated.h"

class UCharacterPawnMovementComponent;

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API ACharacterPawn : public AProjectSolisPawn
{
	GENERATED_BODY()
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CharacterPawn", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterPawnMovementComponent> CharacterPawnMovement;

public:
	ACharacterPawn();

	UCharacterPawnMovementComponent* GetCharacterPawnMovementComponent() const { return CharacterPawnMovement; }

	virtual FCollisionShape GetMovementCollisionShape() const;
	virtual FVector GetMovementCollisionLocation() const;
	virtual FQuat GetMovementCollisionRotation() const;
};
