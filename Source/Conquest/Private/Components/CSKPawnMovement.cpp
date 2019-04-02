// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPawnMovement.h"

UCSKPawnMovement::UCSKPawnMovement()
{
	FocusSlowingRadius = 500.f;

	bIsTravellingToFocusedPoint = false;
	FocusedPoint = FVector(0.f);
	bCanCancelFocusTransition = true;
}

FVector UCSKPawnMovement::ConsumeInputVector()
{
	FVector InputVector = Super::ConsumeInputVector();
	if (InputVector != FVector::ZeroVector)
	{
		// If consuming the input vector, it means the player was allowed to
		// move thus cancel the focus point we might have been travelling to
		bIsTravellingToFocusedPoint = false;
	}

	return Super::ConsumeInputVector();
}

bool UCSKPawnMovement::IsMoveInputIgnored() const
{
	bool bResult = Super::IsMoveInputIgnored();
	if (!bResult)
	{
		// Ignore movement input 
		bResult = (bIsTravellingToFocusedPoint && !bCanCancelFocusTransition);
	}

	return bResult;
}

void UCSKPawnMovement::ApplyControlInputToVelocity(float DeltaTime)
{
	if (bIsTravellingToFocusedPoint)
	{
		FVector DesiredVelocity = FocusedPoint - GetActorLocation();
		
		float DistanceSq = DesiredVelocity.Size2D();
		float RadiusSq = FocusSlowingRadius;

		// Slow down if within the slowing raidus
		if (DistanceSq < RadiusSq)
		{
			float SpeedMod = (DistanceSq / RadiusSq);
			if (FMath::IsNearlyZero(SpeedMod))
			{
				bIsTravellingToFocusedPoint = false;
				Super::ApplyControlInputToVelocity(DeltaTime);

				return;
			}
			
			DesiredVelocity = DesiredVelocity.GetSafeNormal() * (GetMaxSpeed() * (DistanceSq / RadiusSq));
		}
		else
		{
			DesiredVelocity = DesiredVelocity.GetSafeNormal() * GetMaxSpeed();
		}

		FVector Steering = (DesiredVelocity - Velocity);
		Velocity = (Velocity + Steering).GetClampedToMaxSize(GetMaxSpeed());
		//Velocity += (DesiredVelocity - Velocity) * DeltaTime;
		//Velocity = Velocity.GetClampedToMaxSize(GetMaxSpeed());

		// Calling this will update our focus travel flag
		ConsumeInputVector();
	}
	else
	{
		Super::ApplyControlInputToVelocity(DeltaTime);
	}
}

void UCSKPawnMovement::FocusOnPoint(const FVector& Location, bool bCancellable)
{
	if (!bIsTravellingToFocusedPoint || bCanCancelFocusTransition)
	{
		bIsTravellingToFocusedPoint = true;
		FocusedPoint = Location;
		bCanCancelFocusTransition = bCancellable;
	}
}
