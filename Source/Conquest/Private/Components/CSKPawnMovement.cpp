// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPawnMovement.h"

UCSKPawnMovement::UCSKPawnMovement()
{
	bIsTravelling = false;
	bCanCacelTravel = false;
}

FVector UCSKPawnMovement::ConsumeInputVector()
{
	FVector InputVector = Super::ConsumeInputVector();
	if (InputVector != FVector::ZeroVector)
	{
		// If consuming the input vector, it means the player was allowed to
		// move thus cancel the transition goal we might have been travelling to
		bIsTravelling = false;
	}

	return Super::ConsumeInputVector();
}

bool UCSKPawnMovement::IsMoveInputIgnored() const
{
	bool bResult = Super::IsMoveInputIgnored();
	if (!bResult)
	{
		// If executing travel task that can't be cancelled
		bResult = (bIsTravelling && !bCanCacelTravel);
	}

	return bResult;
}

void UCSKPawnMovement::ApplyControlInputToVelocity(float DeltaTime)
{
	if (bIsTravelling)
	{
		UpdateTravelTaskVelocity(DeltaTime);
	}
	else
	{
		Super::ApplyControlInputToVelocity(DeltaTime);
	}
}

void UCSKPawnMovement::TravelToLocation(const FVector& Location, bool bCancellable)
{
	if (!bIsTravelling || bCanCacelTravel)
	{
		bIsTravelling = true;
		bCanCacelTravel = bCancellable;

		TravelFrom = UpdatedComponent->GetComponentLocation();
		TravelGoal = Location;
		TravelElapsedTime = 0.f;
	}
}

// TODO: Moves this to being executed in TickComponent (might need to copy FloatingPawnsMovements Tick and just add it somewhere)
void UCSKPawnMovement::UpdateTravelTaskVelocity(float DeltaTime)
{
	// Estimated time (in seconds) in would take to reach goal if travelling constant velocity of max speed
	const float TravelDilation = 2.f; // TODO: make this a variable?
	TravelElapsedTime = FMath::Clamp(TravelElapsedTime + (DeltaTime / 2.f), 0.f, 1.f);

	float AlphaAlongTack = FMath::InterpSinInOut(0.f, 1.f, TravelElapsedTime);

	// Move along track
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FVector NewLocation = FMath::Lerp(TravelFrom, TravelGoal, AlphaAlongTack);

	// Dividing by delta time here as later in TickComponent, velocity will get multiplied by it (which we want to negate)
	Velocity = (NewLocation - OldLocation) / DeltaTime;

	// Have we finished travelling (at 1, we should be at our destination)
	if (TravelElapsedTime == 1.f)
	{
		bIsTravelling = false;

		// TODO: Could add an event here!
	}

	// Consume input for this frame. This could also cancel out this transition (if cancellable)
	ConsumeInputVector();
}