// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterPawnMovementComponent.h"
#include "../../Libraries/CollisionLibrary.h"
#include "../../Libraries/MathUtilityLibrary.h"
#include "Components/SkeletalMeshComponent.h"

void UCharacterPawnMovementComponent::SetUpdatedComponent(UPrimitiveComponent* Component)
{
	UpdatedComponent = Component;
}

void UCharacterPawnMovementComponent::SetRootMotionMesh(USkeletalMeshComponent* Component)
{
	RootMotionMesh = Component;
	if (!IsValid(RootMotionMesh))
	{
		RootMotionMovementParams = {};
	}
}

void UCharacterPawnMovementComponent::Jump()
{
	// Only valid in walking movement mode.
	if (MovementMode != EKPCMovementMode::Walking)
	{
		return;
	}

	// Apply jump force if grounded.
	if (DetermineIfGrounded(UpdatedComponent->GetCollisionShape(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat()))
	{
		ApplyVerticalForceWalking(JumpZForce);
	}
}

void UCharacterPawnMovementComponent::AddMovementInput(const FVector& Direction, float Scale)
{
	switch (MovementMode)
	{
	case EKPCMovementMode::Walking:
		MovementInputScale = Scale;

		MovementInputDirection = Direction;
		MovementInputDirection.Z = 0.0;
		MovementInputDirection.Normalize();

		break;
	}
}

bool UCharacterPawnMovementComponent::IsGrounded() const
{
	return ((MovementMode == EKPCMovementMode::Walking) &&
		(DetermineIfGrounded(UpdatedComponent->GetCollisionShape(), UpdatedComponent->GetComponentLocation(), UpdatedComponent->GetComponentQuat())));
}

UCharacterPawnMovementComponent::UCharacterPawnMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetTickGroup(ETickingGroup::TG_PostPhysics);
}

void UCharacterPawnMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Gather references.
	World = GetWorld();

	// Tick this component's owning actor after this component. The pawn this component is controlling ticks after the component meaning that on the pawn tick its updated component's 
	// movement for the current frame will have aleady been updated.
	GetOwner()->AddTickPrerequisiteComponent(this);

	// Tick the owning pawn's controller before this component.
	APawn* Pawn = CastChecked<APawn>(GetOwner());
	AddTickPrerequisiteActor(Pawn->GetController());

	// Create movement collision query params that will ensure movement traces ignore the pawn actor.
	MovementCollisionQueryParams = FCollisionQueryParams(NAME_None, MovementTraceComplex, Pawn);

	// Initialize movement input direction.
	MovementInputDirection = UpdatedComponent->GetForwardVector();
}

void UCharacterPawnMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//UE_LOG(LogTemp, Warning, TEXT("Kinematic pawn controller component tick."));

	// Consume root motion data if a root motion mesh has been set. This removes root motion data from the root motion mesh for the current frame.
	if (IsValid(RootMotionMesh))
	{
		RootMotionMovementParams = RootMotionMesh->ConsumeRootMotion();
		if (UAnimInstance* RootMotionAnimInstance = RootMotionMesh->GetAnimInstance())
		{
			bIsAnimMontagePlaying = RootMotionAnimInstance->IsAnyMontagePlaying();
		}
	}

	// Update the pawn's rotation.
	UpdatePawnRotation(DeltaTime);

	FCollisionShape MovementCollisionShape = UpdatedComponent->GetCollisionShape();
	FQuat MovementCollisionRotation = UpdatedComponent->GetComponentQuat();

	// Move the pawn out of collision as moving geometry may have moved into the pawn.
	MoveOutOfCollision(UpdatedComponent->GetComponentLocation(), MovementCollisionRotation, MovementCollisionShape);

	// Tick the component for the current movement mode.
	switch (MovementMode)
	{
	case EKPCMovementMode::Walking: TickMovementModeWalking(DeltaTime, UpdatedComponent->GetComponentLocation(), MovementCollisionShape, MovementCollisionRotation); break;
	}

	// Remove added movement input.
	ClearMovementInput();
}

void UCharacterPawnMovementComponent::TickMovementModeWalking(float DeltaTime, const FVector& MovementCollisionLocation, const FCollisionShape& MovementCollisionShape,
	const FQuat& MovementCollisionRotation)
{
	UpdateComponentAttachment(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);
	UpdateHorizontalMovementWalking(DeltaTime, MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);
	UpdateVerticalMovementWalking(DeltaTime, MovementCollisionShape, UpdatedComponent->GetComponentLocation(), MovementCollisionRotation);
}

void UCharacterPawnMovementComponent::UpdateHorizontalMovementWalking(float Time, const FCollisionShape& MovementCollisionShape, const FVector& MovementCollisionLocation,
	const FQuat& MovementCollisionRotation)
{
	const bool bGroundedBeforeMove = DetermineIfGrounded(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);
	const bool bIsRequestingMovement = IsRequestingMovement();

	FVector HorizontalDisplacement(0.0);

	// If root motion is present for this frame apply translation delta from root bone as displacement for this frame.
	if (HasRootMotion())
	{
		FVector Translation = RootMotionMovementParams.GetRootMotionTransform().GetTranslation();
		Translation.Z = 0.0; // Remove vertical translation.
		HorizontalDisplacement = MovementCollisionRotation.GetForwardVector() * Translation.Length();

		// Calculate initial velocity for the next frame.
		InitialHorizontalVelocityWalking = HorizontalDisplacement / static_cast<double>(Time);
	}
	else
	{
		// Calculate acceleration for this frame.
		FVector Acceleration(0.0);
		FVector Friction(0.0);

		// If requesting movement ensure that the movement input scale is large enough to accelerate up to MinAnalogWalkSpeed.
		if (bIsRequestingMovement)
		{
			MovementInputScale = FMath::Max(static_cast<double>(MinAnalogWalkSpeed / MaxWalkSpeed), MovementInputScale);
		}

		if (bGroundedBeforeMove)
		{
			// On ground so calculate friction in the opposite direction to the current direction the pawn is moving.
			Friction = -InitialHorizontalVelocityWalking * static_cast<double>(FrictionCoefficient * GroundFriction);

			// If not requesting movement and velocity is greater than zero then only apply braking force.
			if ((!bIsRequestingMovement) && (InitialHorizontalVelocityWalking.Length() > 0.0))
			{
				FVector Braking = -InitialHorizontalVelocityWalking.GetSafeNormal() * static_cast<double>(BrakingDecelerationRate);

				Acceleration += Braking;

				if (!bApplySeperateBrakingForce)
				{
					Acceleration += Friction;
				}

				// Stop reversing when backwards acceleration overtakes remaining forward velocity. Take the dot product between new displacement and the current velocity and if the
				// result is below or equal to zero, the pawn will be moving backwards so remove all acceleration and current velocity.
				if (FVector::DotProduct(((InitialHorizontalVelocityWalking * static_cast<double>(Time)) + (0.5 * Acceleration * static_cast<double>(FMath::Square(Time)))).GetSafeNormal(),
					InitialHorizontalVelocityWalking.GetSafeNormal()) <= 0.0)
				{
					Acceleration = FVector::ZeroVector;
					InitialHorizontalVelocityWalking = FVector::ZeroVector;
				}
			}
			else
			{
				// Calculate total acceleration as the sum of movement and friction accelerations.
				Acceleration = ((MovementInputDirection * static_cast<double>(MaxAccelerationRate * MovementInputScale)) + Friction);
			}
		}
		else
		{
			// In air so apply no friction and scale movement force by air control factor.
			Acceleration = (MovementInputDirection * static_cast<double>(MaxAccelerationRate * MovementInputScale * AirControl));
		}

		// Calculate final velocity for this frame.
		FVector FinalHorizontalVelocity = (InitialHorizontalVelocityWalking + (Acceleration * static_cast<double>(Time)));

		// If the final velocity length is greater than the max walk speed limit the acceleration to only be able to reach a length of max walk speed and recalculate final 
		// horizontal velocity.
		if (FinalHorizontalVelocity.Length() > static_cast<double>(MaxWalkSpeed))
		{
			const FVector DesiredVelocity = FinalHorizontalVelocity.GetSafeNormal() * static_cast<double>(MaxWalkSpeed);
			Acceleration = (DesiredVelocity - InitialHorizontalVelocityWalking) / static_cast<double>(Time);
			FinalHorizontalVelocity = (InitialHorizontalVelocityWalking + (Acceleration * static_cast<double>(Time)));
		}

		// Calculate displacement for this frame.
		HorizontalDisplacement = ((FinalHorizontalVelocity + InitialHorizontalVelocityWalking) * 0.5) * static_cast<double>(Time);

		// Set initial velocity for next frame as final velocity on this frame.
		InitialHorizontalVelocityWalking = FinalHorizontalVelocity;
	}

	// Apply displacement for this frame.
	MoveAndSlideHorizontalWalking(HorizontalDisplacement, MovementCollisionLocation, MovementCollisionRotation, MovementCollisionShape);

	// Try to snap down to ground surface if the pawn was grounded at the start of this movement and the move has moved the pawn into an ungrounded state.
	if (bGroundedBeforeMove)
	{
		FVector NewComponentLocation = UpdatedComponent->GetComponentLocation();
		if (!DetermineIfGrounded(MovementCollisionShape, NewComponentLocation, MovementCollisionRotation))
		{
			// Detect if walking off of a ledge. Don't step down if the pawn is walking off of a ledge.
			FHitResult Hit = {};
			FVector LedgeTraceStart = UCollisionLibrary::GetLowestPointOnShape(MovementCollisionShape, NewComponentLocation, MovementCollisionRotation, FVector::UpVector);
			FVector LedgeTraceDelta = FVector(0.0, 0.0, -static_cast<double>(LedgeSearchDistance));
			bool bFoundLedge = !World->LineTraceSingleByChannel(Hit, LedgeTraceStart, LedgeTraceStart + LedgeTraceDelta, MovementTraceChannel, MovementCollisionQueryParams);

			if (!bFoundLedge)
			{
				SnapDownToSurface(MaxStepHeight, NewComponentLocation, MovementCollisionRotation, MovementCollisionShape);
			}
		}
	}
}

void UCharacterPawnMovementComponent::UpdateVerticalMovementWalking(float Time, const FCollisionShape& MovementCollisionShape, const FVector& MovementCollisionLocation,
	const FQuat& MovementCollisionRotation)
{
	// Vertical root motion is not applied in walking mode.

	// Calculate acceleration for this frame.
	FVector VerticalAcceleration = FVector::ZeroVector;
	if (!DetermineIfGrounded(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation))
	{
		VerticalAcceleration += FVector::UpVector * static_cast<double>(CalculateGravity());
	}

	// Calculate final velocity for this frame.
	FVector FinalVerticalVelocity = (InitialVerticalVelocityWalking + (VerticalAcceleration * static_cast<double>(Time)));
	if (FinalVerticalVelocity.Z < 0.0)
	{
		FinalVerticalVelocity = FinalVerticalVelocity.GetClampedToMaxSize(static_cast<double>(MaxFallSpeed));
	}

	// Calculate displacement for this frame.
	FVector VerticalDisplacement = ((FinalVerticalVelocity + InitialVerticalVelocityWalking) * 0.5) * static_cast<double>(Time);

	// Set initial velocity for next frame as final velocity on this frame.
	InitialVerticalVelocityWalking = FinalVerticalVelocity;

	// Apply displacement for this frame.
	MoveAndSlideVerticalWalking(VerticalDisplacement, MovementCollisionLocation, MovementCollisionRotation, MovementCollisionShape);
}

float UCharacterPawnMovementComponent::CalculateGravity()
{
	return (GravityScale * World->GetGravityZ());
}

bool UCharacterPawnMovementComponent::DetermineIfGrounded(const FCollisionShape& MovementCollisionShape, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation) const
{
	FHitResult Hit = FindGroundHit(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);
	if ((!Hit.bBlockingHit) || (!IsWalkableSurface(Hit.ImpactNormal)))
	{
		return false;
	}
	return Hit.bBlockingHit;
}

bool UCharacterPawnMovementComponent::StepUp(const FHitResult& CollisionHitResult, const FVector& Displacement, const FCollisionShape& MovementCollisionShape,
	const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation)
{
	// Check if the collision height is below the maximum step height.
	const double CollisionHeight = UMathUtilityLibrary::NumericalDistance(CollisionHitResult.ImpactPoint.Z,
		UCollisionLibrary::GetLowestPointOnShape(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation, FVector::UpVector).Z);

	if (CollisionHeight > MaxStepHeight)
	{
		return false;
	}

	// Is the step surface walkable?
	if (!IsWalkableSurface(FindStepSurfaceNormalFromCollision(CollisionHitResult)))
	{
		return false;
	}

	// Calculate new sweep start location having teleported the collision shape up to the collision height.
	FVector NewStartLocation = CollisionHitResult.TraceStart + FVector(0.0, 0.0, CollisionHeight + 0.01);

	// Is there a ceiling blocking the teleport up?
	FHitResult HitResult = {};
	if (World->SweepSingleByChannel(HitResult,
		MovementCollisionLocation,
		NewStartLocation,
		MovementCollisionRotation,
		MovementTraceChannel,
		MovementCollisionShape,
		MovementCollisionQueryParams))
	{
		return false;
	}

	// No ceiling blocking the teleport up.

	// Sweep original displacement from the teleported up location.
	HitResult.Reset();
	if (World->SweepSingleByChannel(HitResult,
		NewStartLocation,
		NewStartLocation + Displacement,
		MovementCollisionRotation,
		MovementTraceChannel,
		MovementCollisionShape,
		MovementCollisionQueryParams))
	{
		// Is there enough step available to step on.
		double StepDepth = (FVector(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, 0.0) -
			FVector(CollisionHitResult.ImpactPoint.X, CollisionHitResult.ImpactPoint.Y, 0.0)).Length();

		// Allow step depths below the threshold when the collision height is a small number as this is a collision with a shallow slope that the pawn should walk up.
		if ((StepDepth >= static_cast<double>(MinStepDepth)) || (CollisionHeight < static_cast<double>(StepDepthCollisionHeightThreshold)))
		{
			FVector NewLocation = HitResult.TraceStart + PullBackMovement(HitResult.Location - HitResult.TraceStart);
			UpdatedComponent->SetWorldLocation(NewLocation);
			SnapDownToSurface(MaxSnapDownDistance, NewLocation, MovementCollisionRotation, MovementCollisionShape);

			return true;
		}

		// Not enough step available so fail to step up.

		return false;
	}

	FVector NewLocation = HitResult.TraceEnd;
	UpdatedComponent->SetWorldLocation(NewLocation);
	SnapDownToSurface(MaxSnapDownDistance, NewLocation, MovementCollisionRotation, MovementCollisionShape);

	return true;
}

void UCharacterPawnMovementComponent::SnapDownToSurface(float InMaxSnapDownDistance, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation,
	const FCollisionShape& MovementCollisionShape)
{
	FHitResult HitResult = {};
	if (World->SweepSingleByChannel(HitResult, MovementCollisionLocation, MovementCollisionLocation - FVector(0.0, 0.0, static_cast<double>(InMaxSnapDownDistance)),
		MovementCollisionRotation, MovementTraceChannel, MovementCollisionShape, MovementCollisionQueryParams))
	{
		UpdatedComponent->SetWorldLocation(HitResult.TraceStart + PullBackMovement(HitResult.Location - HitResult.TraceStart));
	}
}

void UCharacterPawnMovementComponent::MoveAndSlideHorizontalWalking(const FVector& Displacement, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation,
	const FCollisionShape& MovementCollisionShape)
{
	// Cap out at max slide iterations.
	FVector CurrentMovementCollisionLocation = MovementCollisionLocation;
	FVector RemainingDisplacement = Displacement;
	FHitResult Hit = {};
	for (int32 i = 0; i < MaxMoveAndSlideIterations; ++i)
	{
		// Early out.
		if (RemainingDisplacement.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
		{
			break;
		}

		// Get ground surface normal.
		const FVector GroundSurfaceNormal = FindGroundSurfaceNormal(CurrentMovementCollisionLocation, MovementCollisionRotation, MovementCollisionShape);
		const bool bIsGroundSurfaceNormalZero = GroundSurfaceNormal.IsNearlyZero(0.01);

		// If the ground normal is zero then the pawn is not grounded so do not adjust displacement vector.
		if (!bIsGroundSurfaceNormalZero)
		{
			// Match displacement vector to the ground normal.
			RemainingDisplacement = UMathUtilityLibrary::MatchVectorToSlope(FVector::UpVector, RemainingDisplacement, GroundSurfaceNormal);
		}

		// Sweep displacement.
		if (!DepenetrateAndSweep(Hit,
			RemainingDisplacement,
			CurrentMovementCollisionLocation,
			MovementCollisionRotation,
			MovementCollisionShape))
		{
			CurrentMovementCollisionLocation = Hit.TraceEnd;
			RemainingDisplacement = FVector::ZeroVector;
			break;
		}

		// Stuck?
		if (Hit.bStartPenetrating)
		{
			RemainingDisplacement = FVector::ZeroVector;
			DrawDebugSphere(World, Hit.TraceStart, 45.0f, 16, FColor::Red);
			continue;
		}

		// Try to step up onto the surface if on the ground.
		if ((!bIsGroundSurfaceNormalZero) && (StepUp(Hit, RemainingDisplacement, MovementCollisionShape, CurrentMovementCollisionLocation, MovementCollisionRotation)))
		{
			// Immediately stop moving and sliding after stepping up. Do not need to update pawn location as this handled by StepUp().
			return;
		}

		// Could not step up so need to slide on the collision surface.

		// Prevent sliding vertically during horizontal movement.
		if (bIsGroundSurfaceNormalZero)
		{
			Hit.Normal.Z = 0.0;
			Hit.Normal.Normalize();
		}
		else
		{
			Hit.Normal = UMathUtilityLibrary::MatchVectorToSlope(GroundSurfaceNormal, Hit.Normal, GroundSurfaceNormal);
		}

		// Update location and remaining displacement.
		CurrentMovementCollisionLocation = Hit.TraceStart + PullBackMovement(Hit.Location - Hit.TraceStart);
		RemainingDisplacement = FVector::VectorPlaneProject(RemainingDisplacement * (1.0 - static_cast<double>(Hit.Time)), Hit.Normal);

		// Prevent the pawn sliding back on itself.
		if (FVector::DotProduct(RemainingDisplacement.GetSafeNormal(), Displacement.GetSafeNormal()) < 0.0)
		{
			RemainingDisplacement = FVector::ZeroVector;
		}
	}

	UpdatedComponent->SetWorldLocation(CurrentMovementCollisionLocation);
}

void UCharacterPawnMovementComponent::MoveAndSlideVerticalWalking(const FVector& Displacement, const FVector& MovementCollisionLocation,
	const FQuat& MovementCollisionRotation, const FCollisionShape& MovementCollisionShape)
{
	// Cap out at max slide iterations.
	FVector CurrentMovementCollisionLocation = MovementCollisionLocation;
	FVector RemainingDisplacement = Displacement;
	FHitResult Hit = {};
	for (int32 i = 0; i < MaxMoveAndSlideIterations; ++i)
	{
		// Early out.
		if (RemainingDisplacement.IsNearlyZero(UE_DOUBLE_KINDA_SMALL_NUMBER))
		{
			break;
		}

		// Sweep displacement.
		if (!DepenetrateAndSweep(Hit,
			RemainingDisplacement,
			CurrentMovementCollisionLocation,
			MovementCollisionRotation,
			MovementCollisionShape))
		{
			CurrentMovementCollisionLocation = Hit.TraceEnd;
			RemainingDisplacement = FVector::ZeroVector;
			break;
		}

		// Stuck?
		if (Hit.bStartPenetrating)
		{
			RemainingDisplacement = FVector::ZeroVector;
			continue;
		}

		// Update location.
		CurrentMovementCollisionLocation = Hit.TraceStart + PullBackMovement(Hit.Location - Hit.TraceStart);

		// Do not slide on ceilings.
		if (Hit.ImpactPoint.Z > CurrentMovementCollisionLocation.Z)
		{
			RemainingDisplacement = FVector::ZeroVector;
			// Zero out vertical velocity for the next frame.
			InitialVerticalVelocityWalking = FVector::ZeroVector;
			break;
		}

		// Do not slide on walkable surfaces and generate landed event.
		if (IsWalkableSurface(Hit.ImpactNormal))
		{
			RemainingDisplacement = FVector::ZeroVector;
			// Zero out vertical velocity for the next frame.
			InitialVerticalVelocityWalking = FVector::ZeroVector;

			OnLandedWalking();

			break;
		}

		// Update remaining displacement.
		RemainingDisplacement = FVector::VectorPlaneProject(RemainingDisplacement * (1.0 - static_cast<double>(Hit.Time)), Hit.Normal);
	}

	UpdatedComponent->SetWorldLocation(CurrentMovementCollisionLocation);
}

bool UCharacterPawnMovementComponent::IsWalkableSurface(const FVector& SurfaceNormal) const
{
	return (UMathUtilityLibrary::VectorAngleDegrees(SurfaceNormal, FVector::UpVector) <= static_cast<double>(MaxWalkableSlopeAngle));
}

FVector UCharacterPawnMovementComponent::FindStepSurfaceNormalFromCollision(const FHitResult& Hit)
{
	FHitResult SlopeHit = {};
	World->SweepSingleByChannel(SlopeHit,
		Hit.ImpactPoint + FVector(0.0, 0.0, 1.0),
		Hit.ImpactPoint - FVector(0.0, 0.0, 0.01),
		FQuat::Identity,
		MovementTraceChannel,
		FCollisionShape::MakeSphere(0.25f),
		MovementCollisionQueryParams);

	return SlopeHit.ImpactNormal;
}

FHitResult UCharacterPawnMovementComponent::FindGroundHit(const FCollisionShape& MovementCollisionShape, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation) const
{
	// Get the center bottom location of the collision shape.
	FVector CenterBottomLocation(MovementCollisionLocation.X,
		MovementCollisionLocation.Y,
		UCollisionLibrary::GetLowestPointOnShape(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation, FVector::UpVector).Z);

	// Get four more points extending from the center bottom location to use as ground sample points.
	FVector SampleLocations[4] =
	{
		CenterBottomLocation + (UpdatedComponent->GetForwardVector() * static_cast<double>(DetermineGroundedSampleMod)),
		CenterBottomLocation - (UpdatedComponent->GetForwardVector() * static_cast<double>(DetermineGroundedSampleMod)),
		CenterBottomLocation + (UpdatedComponent->GetRightVector() * static_cast<double>(DetermineGroundedSampleMod)),
		CenterBottomLocation - (UpdatedComponent->GetRightVector() * static_cast<double>(DetermineGroundedSampleMod))
	};

	// Trace from the center bottom location and additional four locations to search for ground.
	FVector Offset = (FVector::UpVector * static_cast<double>(DetermineGroundedOffset));
	FVector TraceDelta = (FVector::DownVector * static_cast<double>(DetermineGroundedDistance));

	//DrawDebugSphere(World, CenterBottomLocation + Offset, 2.5f, 12, FColor::Red);
	//DrawDebugSphere(World, CenterBottomLocation + TraceDelta, 2.5f, 12, FColor::Green);
	//for (int8 i = 0; i < 4; ++i)
	//{
	//	DrawDebugSphere(World, SampleLocations[i] + Offset, 2.5f, 12, FColor::Red);
	//	DrawDebugSphere(World, SampleLocations[i] + TraceDelta, 2.5f, 12, FColor::Green);
	//}

	FHitResult Hit = {};
	if (!World->LineTraceSingleByChannel(Hit,
		CenterBottomLocation + Offset,
		CenterBottomLocation + TraceDelta,
		MovementTraceChannel,
		MovementCollisionQueryParams))
	{
		for (int8 i = 0; i < 4; ++i)
		{
			Hit.Init();
			if (World->LineTraceSingleByChannel(Hit,
				SampleLocations[i] + Offset,
				SampleLocations[i] + TraceDelta,
				MovementTraceChannel,
				MovementCollisionQueryParams))
			{
				break;
			}
		}
	}

	if (Hit.bBlockingHit)
	{
		//DrawDebugSphere(World, Hit.ImpactPoint, 5.0f, 16, FColor::Blue);

		return Hit;
	}

	// Fallback on shape cast if sample points fail to find ground collision. The pawn may still be grounded especially if the movement collision is longer than it is tall.
	//UCollisionLibrary::DrawDebugShape(World, MovementCollisionLocation, MovementCollisionShape, MovementCollisionRotation, FColor::Red);
	//UCollisionLibrary::DrawDebugShape(World, MovementCollisionLocation + TraceDelta, MovementCollisionShape, MovementCollisionRotation, FColor::Green);

	Hit.Init();
	if (World->SweepSingleByChannel(Hit,
		MovementCollisionLocation,
		MovementCollisionLocation + TraceDelta,
		MovementCollisionRotation,
		MovementTraceChannel,
		MovementCollisionShape,
		MovementCollisionQueryParams))
	{
		//UCollisionLibrary::DrawDebugShape(World, Hit.Location, MovementCollisionShape, MovementCollisionRotation, FColor::Blue);

		return Hit;
	}

	Hit.Init();
	return Hit;
}

FVector UCharacterPawnMovementComponent::FindGroundSurfaceNormal(const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation, const FCollisionShape& MovementCollisionShape)
{
	FHitResult Hit = FindGroundHit(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);
	if (IsWalkableSurface(Hit.ImpactNormal))
	{
		return Hit.ImpactNormal;
	}
	return FVector::ZeroVector;
}

void UCharacterPawnMovementComponent::UpdateComponentAttachment(const FCollisionShape& MovementCollisionShape, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation)
{
	// Check for walkable surfaces below the pawn and attach the updated component to it if one is found. This is to support sticking to walkable moving geometry such as an elevator or 
	// moving platform.
	FHitResult Hit = FindGroundHit(MovementCollisionShape, MovementCollisionLocation, MovementCollisionRotation);

	if ((!Hit.bBlockingHit) || (!IsWalkableSurface(Hit.ImpactNormal)))
	{
		UpdatedComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
		return;
	}

	UpdatedComponent->AttachToComponent(Hit.Component.Get(), FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
}

FVector UCharacterPawnMovementComponent::AdjustDepenetrationNormalWalking(FVector Normal, const FVector& ImpactNormal)
{
	return ((IsWalkableSurface(ImpactNormal)) ? FVector::UpVector : Normal);
}

void UCharacterPawnMovementComponent::ApplyRootMotionRotationWalking(const FQuat& Rotation)
{
	// Remove pitch and roll rotation components in walking mode.
	UpdatedComponent->AddWorldRotation(FRotator(0.0, RootMotionMovementParams.GetRootMotionTransform().GetRotation().Rotator().Yaw, 0.0));
}

FRotator UCharacterPawnMovementComponent::GetMovementOrientationWalking()
{
	return InitialHorizontalVelocityWalking.ToOrientationRotator();
}

void UCharacterPawnMovementComponent::ApplyVerticalForceWalking(float Force)
{
	InitialVerticalVelocityWalking = FVector::ZeroVector; // Remove this line to make the pawn need to overcome any existing vertical velocity with the added force.

	// Displacement initial is square root of -2 multiplied by acceleration multiplied by displacement. Vf^2 = Vi^2 + 2ad rearranged for Vi when Vf is 0 (the apex of the jump).
	// The true equation would be FMath::Sqrt(-2.0 * ScaledPawnGravity * JumpHeightInCm).
	// Use world gravity Z unscaled by character pawn movement component's gravity scale here to allow the character pawn to be able to jump when gravity scale is set to 0.
	// Not mathematically correct but the displacement value (jump height) acts as a jump force/strength value.
	InitialVerticalVelocityWalking += FVector::UpVector * FMath::Sqrt(-2.0 * static_cast<double>(World->GetGravityZ()) * static_cast<double>(Force));
}

FVector UCharacterPawnMovementComponent::GetVelocityWalking() const
{
	return InitialHorizontalVelocityWalking + InitialVerticalVelocityWalking;
}

void UCharacterPawnMovementComponent::OnLandedWalking()
{
	if (bRemoveVelocityOnLand)
	{
		// Remove any remaining horizontal velocity to stop the pawn having to brake to a stop after landing when there is no movement input.
		if (MovementInputScale < 0.01)
		{
			InitialHorizontalVelocityWalking = FVector::ZeroVector;
		}
	}
}

bool UCharacterPawnMovementComponent::DepenetrateAndSweep(FHitResult& OutHit, const FVector& Displacement, const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation,
	const FCollisionShape& MovementCollisionShape)
{
	HitResultScratch.Reset();

	// Sweep with inflated skin.
	const bool bSkinHit = World->SweepMultiByChannel(HitResultScratch,
		MovementCollisionLocation,
		MovementCollisionLocation + Displacement,
		MovementCollisionRotation,
		MovementTraceChannel,
		UCollisionLibrary::InflateShape(MovementCollisionShape, SweepShapeInflationAmount),
		MovementCollisionQueryParams);

	if (!bSkinHit)
	{
		OutHit.Init();
		OutHit.TraceStart = MovementCollisionLocation;
		OutHit.TraceEnd = MovementCollisionLocation + Displacement;
		OutHit.Location = MovementCollisionLocation + Displacement;
		OutHit.Time = 1.0f;
		return false;
	}

	// Have initial overlaps?
	int32 NumInitialOverlaps = 0;
	for (const FHitResult& It : HitResultScratch)
	{
		if (It.bStartPenetrating)
		{
			++NumInitialOverlaps;
		}
	}
	if (NumInitialOverlaps == 0)
	{
		OutHit = HitResultScratch.Last();
		return bSkinHit;
	}

	// Iteratively resolve penetration.
	FVector Fixup(0.0);
	for (int32 i = 0; i < MaxPenetrationResolutionIterations; ++i)
	{
		double ErrorSum = 0.0;
		for (const FHitResult& It : HitResultScratch)
		{
			if (It.bStartPenetrating)
			{
				// Take the dot product of the fixup and the normal to determine how much of the penetration has already been taken care of.
				const double Error = FMath::Max(0.0, (It.PenetrationDepth + static_cast<double>(AdditionalDepenetrationDistance)) - (FVector::DotProduct(Fixup, It.Normal)));
				ErrorSum += Error;
				Fixup += Error * It.Normal;
			}
		}
		// Stop iterating if a solution has been found.
		if (ErrorSum < UE_DOUBLE_KINDA_SMALL_NUMBER)
		{
			break;
		}
	}

	// Resweep from new start.
	return World->SweepSingleByChannel(OutHit, MovementCollisionLocation + Fixup, MovementCollisionLocation + Fixup + Displacement, MovementCollisionRotation,
		MovementTraceChannel, MovementCollisionShape, MovementCollisionQueryParams);
}

void UCharacterPawnMovementComponent::MoveOutOfCollision(const FVector& MovementCollisionLocation, const FQuat& MovementCollisionRotation, const FCollisionShape& MovementCollisionShape)
{
	HitResultScratch.Reset();

	// Sweep forwards a small distance.
	if (!World->SweepMultiByChannel(HitResultScratch,
		MovementCollisionLocation,
		MovementCollisionLocation + (UpdatedComponent->GetForwardVector() * 0.01),
		MovementCollisionRotation,
		MovementTraceChannel,
		MovementCollisionShape,
		MovementCollisionQueryParams))
	{
		return;
	}

	// Have initial overlaps?
	int32 NumInitialOverlaps = 0;
	for (const FHitResult& It : HitResultScratch)
	{
		if (It.bStartPenetrating)
		{
			++NumInitialOverlaps;
		}
	}
	if (NumInitialOverlaps == 0)
	{
		return;
	}

	// Iteratively resolve penetration.
	FVector Fixup(0.0);
	for (int32 i = 0; i < MaxPenetrationResolutionIterations; ++i)
	{
		double ErrorSum = 0.0;
		for (const FHitResult& It : HitResultScratch)
		{
			if (It.bStartPenetrating)
			{
				FVector Normal = It.Normal;

				switch (MovementMode)
				{
				case EKPCMovementMode::Walking: Normal = AdjustDepenetrationNormalWalking(Normal, It.ImpactNormal); break;
				}

				// Take the dot product of the fixup and the normal to determine how much of the penetration has already been taken care of.
				const double Error = FMath::Max(0.0, (It.PenetrationDepth + static_cast<double>(AdditionalDepenetrationDistance)) - (FVector::DotProduct(Fixup, Normal)));
				ErrorSum += Error;
				Fixup += Error * Normal;
			}
		}
		// Stop iterating if a solution has been found.
		if (ErrorSum < UE_DOUBLE_KINDA_SMALL_NUMBER)
		{
			break;
		}
	}

	UpdatedComponent->SetWorldLocation(MovementCollisionLocation + Fixup);
}

double UCharacterPawnMovementComponent::CalculateOrientRotationComponentDelta(double Current, double Target, float DeltaTime, float Speed)
{
	const double Dist = static_cast<double>((((FMath::TruncToInt(Target - Current)) + 540) % 360) - 180);
	const double Step = static_cast<double>(DeltaTime) * static_cast<double>(Speed);
	return FMath::Clamp(Dist, -Step, Step);
}

void UCharacterPawnMovementComponent::AddVerticalForce(float Force)
{
	switch (MovementMode)
	{
	case EKPCMovementMode::Walking: ApplyVerticalForceWalking(Force); break;
	}
}

FVector UCharacterPawnMovementComponent::GetVelocity() const
{
	switch (MovementMode)
	{
	case EKPCMovementMode::Walking: return GetVelocityWalking();
	}
	return FVector::ZeroVector;
}

void UCharacterPawnMovementComponent::UpdatePawnRotation(float DeltaTime)
{
	// If root motion is present rotate the pawn with root motion instead of movement input unless rotation from movement input has been requested.
	if (HasRootMotion())
	{
		switch (MovementMode)
		{
		case EKPCMovementMode::Walking: ApplyRootMotionRotationWalking(RootMotionMovementParams.GetRootMotionTransform().GetRotation()); break;
		}

		if (!bAllowMovementRotationDuringRootMotion)
		{
			return;
		}
	}

	if (!bOrientRotationToMovement)
	{
		return;
	}

	// Only orient rotation to movement when there is movement input.
	if (!IsRequestingMovement())
	{
		return;
	}

	FRotator CurrentRotation = UpdatedComponent->GetComponentRotation();

	FRotator MovementOrientation(0.0);
	switch (MovementMode)
	{
	case EKPCMovementMode::Walking: MovementOrientation = GetMovementOrientationWalking(); break;
	}

	FRotator NewRotation(((bOrientPitch) ? MovementOrientation.Pitch : CurrentRotation.Pitch),
		((bOrientYaw) ? MovementOrientation.Yaw : CurrentRotation.Yaw),
		((bOrientRoll) ? MovementOrientation.Roll : CurrentRotation.Roll));

	UpdatedComponent->SetWorldRotation(FRotator(
		CurrentRotation.Pitch + CalculateOrientRotationComponentDelta(CurrentRotation.Pitch, NewRotation.Pitch, DeltaTime, OrientRotationRate.Pitch),
		CurrentRotation.Yaw + CalculateOrientRotationComponentDelta(CurrentRotation.Yaw, NewRotation.Yaw, DeltaTime, OrientRotationRate.Yaw),
		CurrentRotation.Roll + CalculateOrientRotationComponentDelta(CurrentRotation.Roll, NewRotation.Roll, DeltaTime, OrientRotationRate.Roll)));
}

void UCharacterPawnMovementComponent::ClearMovementInput()
{
	// Do not clear movement input direction as the last direction is used to update the pawn rotation when there is no movement input being added.
	//MovementInputDirection = FVector::ZeroVector;

	MovementInputScale = 0.0f;
}

bool UCharacterPawnMovementComponent::IsRequestingMovement()
{
	return (MovementInputScale > 0.0f);
}

bool UCharacterPawnMovementComponent::HasRootMotion()
{
	return RootMotionMovementParams.bHasRootMotion;
}

FVector UCharacterPawnMovementComponent::PullBackMovement(const FVector& Movement)
{
	const double Distance = Movement.Length();
	return (Distance > static_cast<double>(PullBackMovementEpsilon)) ? Movement * ((Distance - static_cast<double>(PullBackMovementEpsilon)) / Distance) : FVector(0.0);
}
