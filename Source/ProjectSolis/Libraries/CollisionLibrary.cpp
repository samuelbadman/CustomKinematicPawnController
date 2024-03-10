// Fill out your copyright notice in the Description page of Project Settings.


#include "CollisionLibrary.h"
#include "MathUtilityLibrary.h"

FCollisionShape UCollisionLibrary::InflateShape(const FCollisionShape& Shape, float Amount)
{
	switch (Shape.ShapeType)
	{
	case ECollisionShape::Capsule:
		return FCollisionShape::MakeCapsule(Shape.GetCapsuleRadius() + Amount, Shape.GetCapsuleHalfHeight() + Amount);

	case ECollisionShape::Sphere:
		return FCollisionShape::MakeSphere(Shape.GetSphereRadius() + Amount);

	case ECollisionShape::Box:
		return FCollisionShape::MakeBox(Shape.GetBox() + Amount);

	default:
		return Shape;
	}
}

FVector UCollisionLibrary::GetLowestPointOnShape(const FCollisionShape& Shape,
	const FVector& ShapeLocation,
	const FQuat& ShapeRotation,
	const FVector& ShapeUp)
{
	switch (Shape.ShapeType)
	{
	case ECollisionShape::Type::Capsule:
		return GetLowestPointOnShape_Capsule(ShapeLocation, ShapeUp, Shape.GetCapsuleAxisHalfLength(), Shape.GetCapsuleRadius());

	case ECollisionShape::Type::Sphere:
		return GetLowestPointOnShape_Sphere(ShapeLocation, Shape.GetSphereRadius());

	case ECollisionShape::Type::Box:
		return GetLowestPointOnShape_Box(ShapeLocation, ShapeRotation, Shape.GetExtent());

	default:
		return ShapeLocation;
	}
}

FVector UCollisionLibrary::GetLowestPointOnShape_Capsule(const FVector& Location,
	const FVector& Up,
	const float HalfHeight_WithoutHemisphere,
	const float Radius)
{
	return Location - // CollisionLocation at the capsule's world location.
		(Up * HalfHeight_WithoutHemisphere * FMath::Sign(FVector::DotProduct(Up, FVector::UpVector))) - // Move to the center of the lower sphere cap.
		(FVector::UpVector * Radius); // Go downwards by the capsule sphere cap radius amount.
}

FVector UCollisionLibrary::GetLowestPointOnShape_Sphere(const FVector& Location, const float Radius)
{
	FVector Result = Location;
	Result.Z -= Radius;
	return Result;
}

FVector UCollisionLibrary::GetLowestPointOnShape_Box(const FVector& Location, const FQuat& Rotation, const FVector& Extent)
{
	static const FVector BoxPointMapping[8] =
	{
		FVector(1.0, 1.0, 1.0),
		FVector(1.0, 1.0, -1.0),
		FVector(1.0, -1.0, 1.0),
		FVector(1.0, -1.0, -1.0),
		FVector(-1.0, 1.0, 1.0),
		FVector(-1.0, 1.0, -1.0),
		FVector(-1.0, -1.0, 1.0),
		FVector(-1.0, -1.0, -1.0)
	};

	FVector Lowest(TNumericLimits<double>::Max());
	for (uint8 BoxPointIter = 0; BoxPointIter < 8; BoxPointIter++)
	{
		const FVector Point = Location + ((Rotation * BoxPointMapping[BoxPointIter]) * Extent);
		if (Point.Z < Lowest.Z)
		{
			Lowest = Point;
		}
	}
	return Lowest;
}

double UCollisionLibrary::GetShapeHalfHeight(const FVector& ShapeLocation, const FVector& ShapeLowestPoint)
{
	return UMathUtilityLibrary::NumericalDistance(ShapeLocation.Z, ShapeLowestPoint.Z);
}

void UCollisionLibrary::DrawDebugShape(UWorld* const InWorld,
	const FVector& Center,
	const FCollisionShape& Shape,
	const FQuat& ShapeRotation,
	const FColor& Color,
	const float LifeTime,
	const float Thickness)
{
	switch (Shape.ShapeType)
	{
	case ECollisionShape::Capsule:
		DrawDebugCapsule(InWorld, Center, Shape.GetCapsuleHalfHeight(), Shape.GetCapsuleRadius(), ShapeRotation, Color, false, LifeTime, 0, Thickness);
		break;
	case ECollisionShape::Box:
		DrawDebugBox(InWorld, Center, Shape.GetExtent(), ShapeRotation, Color, false, LifeTime, 0, Thickness);
		break;
	case ECollisionShape::Sphere:
	{
		static constexpr int32 SphereSegments = 16;
		DrawDebugSphere(InWorld, Center, Shape.GetSphereRadius(), SphereSegments, Color, false, LifeTime, 0, Thickness);
		break;
	}
	default:
		break;
	}
}

bool UCollisionLibrary::SweepShapeSingleByChannel(UWorld* const World,
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	const ECollisionChannel TraceChannel,
	const FQuat& ShapeRotation,
	const FCollisionShape& Shape,
	const FCollisionQueryParams& CollisionQueryParams,
	const bool DrawDebug,
	const float DebugDuration)
{
	const bool bHit = World->SweepSingleByChannel(OutHit, Start, End, ShapeRotation, TraceChannel, Shape, CollisionQueryParams);

#if !UE_BUILD_SHIPPING
	if (DrawDebug)
	{
		static const FColor MissColor = FColor::Red;
		static const FColor HitColor = FColor::Green;
		static const FColor ImpactPointColor = FColor::Red;
		static const FColor ImpactNormalColor = FColor::Green;
		static const FColor NormalColor = FColor::Blue;

		DrawDebugShape(World, OutHit.TraceStart, Shape, ShapeRotation, MissColor, DebugDuration);

		if (bHit)
		{
			DrawDebugShape(World, OutHit.Location, Shape, ShapeRotation, HitColor, DebugDuration);
			DrawDebugSphere(World, OutHit.ImpactPoint, 5.0f, 16, ImpactPointColor, false, DebugDuration);
			DrawDebugDirectionalArrow(World, OutHit.ImpactPoint, OutHit.ImpactPoint + (OutHit.ImpactNormal * 25.0), 12.0f, ImpactNormalColor, false, DebugDuration, 1);
			DrawDebugDirectionalArrow(World, OutHit.ImpactPoint, OutHit.ImpactPoint + (OutHit.Normal * 25.0), 12.0f, NormalColor, false, DebugDuration, 0);
		}
		else
		{
			DrawDebugShape(World, OutHit.TraceEnd, Shape, ShapeRotation, MissColor, DebugDuration);
		}
	}
#endif

	return bHit;
}

bool UCollisionLibrary::SweepShapeMultiByChannel(UWorld* const World,
	TArray<FHitResult>& OutHits,
	const FVector& Start,
	const FVector& End,
	const ECollisionChannel TraceChannel,
	const FQuat& ShapeRotation,
	const FCollisionShape& Shape,
	const FCollisionQueryParams& CollisionQueryParams,
	const bool DrawDebug,
	const float DebugDuration)
{
	const bool bHit = World->SweepMultiByChannel(OutHits, Start, End, ShapeRotation, TraceChannel, Shape, CollisionQueryParams);

#if !UE_BUILD_SHIPPING
	if (DrawDebug)
	{
		static const FColor MissColor = FColor::Red;
		static const FColor HitColor = FColor::Green;
		static const FColor ImpactPointColor = FColor::Red;
		static const FColor ImpactNormalColor = FColor::Green;
		static const FColor NormalColor = FColor::Blue;

		DrawDebugShape(World, Start, Shape, ShapeRotation, MissColor, DebugDuration);

		for (const FHitResult& Hit : OutHits)
		{
			DrawDebugShape(World, Hit.Location, Shape, ShapeRotation, HitColor, DebugDuration);
			DrawDebugSphere(World, Hit.ImpactPoint, 5.0f, 16, ImpactPointColor, false, DebugDuration);
			DrawDebugDirectionalArrow(World, Hit.ImpactPoint, Hit.ImpactPoint + (Hit.ImpactNormal * 25.0), 12.0f, ImpactNormalColor, false, DebugDuration, 1);
			DrawDebugDirectionalArrow(World, Hit.ImpactPoint, Hit.ImpactPoint + (Hit.Normal * 25.0), 12.0f, NormalColor, false, DebugDuration, 0);
		}

		if (OutHits.IsEmpty())
		{
			DrawDebugShape(World, End, Shape, ShapeRotation, MissColor, DebugDuration);
		}
	}
#endif

	return bHit;
}
