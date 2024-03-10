// Fill out your copyright notice in the Description page of Project Settings.


#include "MathUtilityLibrary.h"

double UMathUtilityLibrary::VectorAngleRadians(const FVector& A, const FVector& B)
{
	check((A.IsZero()) ? true : A.IsNormalized());
	check((B.IsZero()) ? true : B.IsNormalized());
	return FMath::Acos(FVector::DotProduct(A, B));
}

double UMathUtilityLibrary::VectorAngleDegrees(const FVector& A, const FVector& B)
{
	return FMath::RadiansToDegrees(VectorAngleRadians(A, B));
}

double UMathUtilityLibrary::DragEquation(const float Density, const double Velocity, const float DragCoefficient, const float CrossSectionalArea)
{
	return 0.5 * static_cast<double>(Density) * (Velocity * Velocity) * static_cast<double>(DragCoefficient) * static_cast<double>(CrossSectionalArea);
}

FVector UMathUtilityLibrary::DragEquation(const float Density, const FVector& Velocity, const float DragCoefficient, const float CrossSectionalArea)
{
	// Density = The density of the fluid the shape is falling through in kg/cm3. On earth pure, dry air has a density of 1.293e-6kg/cm3.

	// DragCoefficient = Value used to quantify the drag or resistance of an object in a fluid environment. A vertical human in free fall has a coefficient of 0.7. 
	// A horizontal human in free fall has a coefficient of 1.0. A higher value means the object has more resistance while a lower value means the 
	// object is more streamlined and has less resistance.

	// CrossSectionalArea = The cross sectional area of the shape looked at in the direction drag is being applied.

	return 0.5 * static_cast<double>(Density) * (Velocity * Velocity) * static_cast<double>(DragCoefficient) * static_cast<double>(CrossSectionalArea);
}

float UMathUtilityLibrary::CircleArea(const float R)
{
	return UE_PI * (R * R);
}

double UMathUtilityLibrary::NumericalDistance(const double A, const double B)
{
	return FMath::Abs(A - B);
}

FVector UMathUtilityLibrary::MatchVectorToSlope(const FVector& Up, const FVector& Vector, const FVector& Normal)
{
	const FVector Right = FVector::CrossProduct(Up, Vector.GetSafeNormal()).GetSafeNormal();
	return FVector::CrossProduct(Right, Normal).GetSafeNormal() * Vector.Length();
}
