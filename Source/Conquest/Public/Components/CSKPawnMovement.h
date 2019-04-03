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

private:

	/** Updates velocity to travel towards target destination */
	void UpdateTravelTaskVelocity(float DeltaTime);

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
};
