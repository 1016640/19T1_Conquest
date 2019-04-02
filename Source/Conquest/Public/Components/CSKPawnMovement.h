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

	/** Set a point to move to over time. If not cancellable, 
	calling this function again will be ignored until finished */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void FocusOnPoint(const FVector& Location, bool bCancellable = true);

public:

	/** The radius at which to start slowing down when moving to focused point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CSKPawnMovement, meta = (ClampMin = 0))
	float FocusSlowingRadius;

protected:

	/** If we are currently moving to a focused position */
	uint32 bIsTravellingToFocusedPoint : 1;

	/** The focused point to move to */
	FVector FocusedPoint;

	/** If the focus transition can be cancelled */
	uint32 bCanCancelFocusTransition : 1;
};
