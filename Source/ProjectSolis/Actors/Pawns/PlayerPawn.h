// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterPawn.h"
#include "PlayerPawn.generated.h"

class UCapsuleComponent;
class UArrowComponent;

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API APlayerPawn : public ACharacterPawn
{
	GENERATED_BODY()
	
private:
	UPROPERTY(VisibleAnywhere, Category = "PlayerPawn")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "PlayerPawn")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "PlayerPawn")
	TObjectPtr<UArrowComponent> Arrow;

public:
	APlayerPawn();

	virtual FCollisionShape GetMovementCollisionShape() const override;
	virtual FVector GetMovementCollisionLocation() const override;
	virtual FQuat GetMovementCollisionRotation() const override;

protected:
	inline UCapsuleComponent* GetCapsuleComponent() const { return Capsule; }

private:
	virtual void Tick(float DeltaTime) override;
};
