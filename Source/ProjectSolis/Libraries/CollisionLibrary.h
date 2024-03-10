// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSolisLibrary.h"
#include "CollisionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API UCollisionLibrary : public UProjectSolisLibrary
{
	GENERATED_BODY()
	
public:
	static FCollisionShape InflateShape(const FCollisionShape& Shape, float Amount);

	static FVector GetLowestPointOnShape(const FCollisionShape& Shape,
		const FVector& ShapeLocation,
		const FQuat& ShapeRotation,
		const FVector& ShapeUp);

	static FVector GetLowestPointOnShape_Capsule(const FVector& Location,
		const FVector& Up,
		const float HalfHeight_WithoutHemisphere,
		const float Radius);

	static FVector GetLowestPointOnShape_Sphere(const FVector& Location, const float Radius);

	static FVector GetLowestPointOnShape_Box(const FVector& Location, const FQuat& Rotation, const FVector& Extent);

	static double GetShapeHalfHeight(const FVector& ShapeLocation, const FVector& ShapeLowestPoint);

	static void DrawDebugShape(UWorld* const InWorld,
		const FVector& Center,
		const FCollisionShape& Shape,
		const FQuat& ShapeRotation,
		const FColor& Color,
		const float LifeTime = 0.0f,
		const float Thickness = 0.0f);

	static bool SweepShapeSingleByChannel(UWorld* const World,
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		const ECollisionChannel TraceChannel,
		const FQuat& ShapeRotation,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& CollisionQueryParams,
		const bool DrawDebug = false,
		const float DebugDuration = 0.0f);

	static bool SweepShapeMultiByChannel(UWorld* const World,
		TArray<FHitResult>& OutHits,
		const FVector& Start,
		const FVector& End,
		const ECollisionChannel TraceChannel,
		const FQuat& ShapeRotation,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& CollisionQueryParams,
		const bool DrawDebug = false,
		const float DebugDuration = 0.0f);
};
