// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectPawn.h"

// Sets default values
AProjectPawn::AProjectPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AProjectPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AProjectPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AProjectPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

