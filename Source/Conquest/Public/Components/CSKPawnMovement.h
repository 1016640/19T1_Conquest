// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "CSKPawnMovement.generated.h"

/**
 * Movement component to be used by CSK pawns
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API UCSKPawnMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:

	UCSKPawnMovement();

public:

	// Begin UPawnMovementComponent Interface
	virtual void AddInputVector(FVector WorldVector, bool bForce) override;
	virtual FVector ConsumeInputVector() override;
	virtual bool IsMoveInputIgnored() const override;
	// End UPawnMovementComponent Interface

protected:

	// Begin UFloatingPawnMovement Interface
	virtual void ApplyControlInputToVelocity(float DeltaTime) override;
	// End UFloatingPawnMovement Interface

public:

	/** Travels to the given point overtime from our owners current location */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void TravelToLocation(const FVector& Location, bool bCancellable = true);

	/** Set the actor to track, setting this to null means to track no actor.
	Tracking actors takes priority over all other forms of movement, so to restore
	free movement, this function should be called with a null actor */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void TrackActor(AActor* ActorToTrack, bool bIgnoreIfStatic = false);

	/** Stops tracking our current actor */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void StopTrackingActor();

public:

	/** If we are currently following an actor move around */
	FORCEINLINE bool IsTrackingActor() const { return TrackedActor != nullptr; }

private:

	/** Updates velocity to travel towards target destination */
	void UpdateTravelTaskVelocity(float DeltaTime);

	/** Updates velocity to move with tracked actor */
	void UpdateTrackTaskVelocity(float DeltaTime);

private:

	/** If we are currently transitioning to a designated point */
	uint32 bIsTravelling : 1;

	/** If the transition can be cancelled */
	uint32 bCanCacelTravel : 1;

	/** The location to transitioning from */
	FVector TravelFrom;

	/** The location to transition to */
	FVector TravelGoal;

	/** The time we have been travelling for */
	float TravelElapsedTime;
	
	/** The actor we are tracking */
	UPROPERTY(Transient, DuplicateTransient)
	AActor* TrackedActor;
};
