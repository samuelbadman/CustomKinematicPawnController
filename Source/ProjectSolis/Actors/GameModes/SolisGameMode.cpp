// Fill out your copyright notice in the Description page of Project Settings.


#include "SolisGameMode.h"
#include "../Cameras/CameraActorBase.h"

void ASolisGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultCameraActor();
}

void ASolisGameMode::SpawnDefaultCameraActor()
{
	UClass* CameraActorClass = DefaultCameraActorClass.LoadSynchronous();
	if (!IsValid(CameraActorClass))
	{
		return;
	}

	FActorSpawnParameters SpawnParameters = {};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CameraActor = GetWorld()->SpawnActor<ACameraActorBase>(CameraActorClass->GetDefaultObject()->GetClass(), SpawnParameters);
}
