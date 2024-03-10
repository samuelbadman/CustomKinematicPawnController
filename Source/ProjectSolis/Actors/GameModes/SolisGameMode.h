// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSolisGameModeBase.h"
#include "SolisGameMode.generated.h"

class ACameraActorBase;

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API ASolisGameMode : public AProjectSolisGameModeBase
{
	GENERATED_BODY()
	
private:
	// Default spawned camera actor class.
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSoftClassPtr<ACameraActorBase> DefaultCameraActorClass = nullptr;

	ACameraActorBase* CameraActor = nullptr;

private:
	virtual void BeginPlay() override;

	void SpawnDefaultCameraActor();
};
