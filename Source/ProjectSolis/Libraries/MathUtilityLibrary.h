// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProjectSolisLibrary.h"
#include "MathUtilityLibrary.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSOLIS_API UMathUtilityLibrary : public UProjectSolisLibrary
{
	GENERATED_BODY()
	
public:
	// Returns the angle between A and B in radians. A and B must be normalized.
	static double VectorAngleRadians(const FVector& A, const FVector& B);

	// Returns the angle between A and B in degrees. A and B must be normalized.
	static double VectorAngleDegrees(const FVector& A, const FVector& B);

	// Returns the calculated drag velocity.
	static double DragEquation(const float Density, const double Velocity, const float DragCoefficient, const float CrossSectionalArea);

	// Returns the calculated drag velocity.
	static FVector DragEquation(const float Density, const FVector& Velocity, const float DragCoefficient, const float CrossSectionalArea);

	// Returns the area of the circle with radius R.
	static float CircleArea(const float R);

	// Returns the numerical distance between A and B.
	static double NumericalDistance(const double A, const double B);

	// Returns the input vector rotated to match the slope plane with input normal. Returned vector retains the original length.
	static FVector MatchVectorToSlope(const FVector& Up, const FVector& Vector, const FVector& Normal);
};
