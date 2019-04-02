// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Pawn.h"
#include "CSKPawn.generated.h"

class UCameraComponent;
class UCSKPawnMovement;
class UCSKSpringArmComponent;
class USphereComponent;


/**
 * Default pawn used by players to traverse around the board. This pawn does not represent
 * the players position on the board, but acts as the spectating view the player controls
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPawn : public APawn
{
	GENERATED_BODY()

public:

	ACSKPawn();

protected:

	// Begin APawn Interface
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// End APawn Interface

protected:

	// TODO: Move forward and move right might get changed by moving the mouse towards the edges of the screen

	/** Moves this pawn forward based on view rotation */
	void MoveForward(float Value);

	/** Moves this pawn right based on view rotation */
	void MoveRight(float Value);

	/** Zooms in or out the camera */
	void ZoomCamera(float Value);

public:

	/** Get sphere collision component */
	FORCEINLINE USphereComponent* GetSphereCollision() const { return SphereCollision; }

	/** Get camera boom component */
	FORCEINLINE UCSKSpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Get camera component */
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }

	/** Get pawn movement component */
	FORCEINLINE UCSKPawnMovement* GetPawnMovement() const { return PawnMovement; }

private:

	/** Sphere used by to act as collisions against boundaries */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereCollision;

	/** Spring arm that acts as a camera boom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UCSKSpringArmComponent* CameraBoom;

	/** Players viewport of the world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** Movement component for traversing the board */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UCSKPawnMovement* PawnMovement;

public:

	/** Moves this pawn to focus on given location. The move can
	optionally be cancellable by the player inputting movement */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void FocusOnPoint(const FVector& Location, bool bCancellable = true);
};
