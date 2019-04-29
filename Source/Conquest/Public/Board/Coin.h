// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "Coin.generated.h"

class UCurveFloat;
class UStaticMeshComponent;

/** Delegate for when a coin has finished the flip sequence. Get what side the coin landed on */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoinFlipFinished, bool, bHeads);

/** 
 * The coin to flip when determining which player goes first
*/
UCLASS(abstract)
class CONQUEST_API ACoin : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ACoin();

public:

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

	// Begin UObject Interface
	virtual void PostInitProperties() override;
	// End UObject Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Get coins mesh */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

private:

	/** The mesh of the coin */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	/** Flips this coin */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = Coin)
	void Flip();

public:

	/** If this coin is currently performing the flip */
	UFUNCTION(BlueprintPure, Category = Coin)
	bool IsCoinFlipping() const;

	/** Get the ID of the player who won the coin flip (-1 if no winner) */
	UFUNCTION(BlueprintPure, Category = Coin)
	int32 GetWinnersPlayerID() const;

private:

	/** Constructs the timeline to use */
	void ConstructTimeline();

	/** Gets if more than half of the timeline has played */
	bool HasHalfOfTimelinePlayed() const;

	/** Handles update to location track */
	UFUNCTION()
	void OnLocationInterpUpdated(float Value);

	/** Handles update to rotation track */
	UFUNCTION()
	void OnRotationInterpUpdated(float Value);

	/** Handles when flip timeline has finished */
	UFUNCTION()
	void OnFlipFinished();

public:

	/** Event for when this coin has finished the flip sequence.
	This is called on both the server and the client */
	UPROPERTY(BlueprintAssignable)
	FCoinFlipFinished OnCoinFlipComplete;

protected:

	/** The interpolation track for the coins vertical jump. This should both start and end at 0 */
	UPROPERTY(EditDefaultsOnly, Category = Coin)
	UCurveFloat* LocationCurve;

	/** The interpolation track for the coins rotation. This only applies to the coins pitch. 0 to 1 represents a full 360 loop */
	UPROPERTY(EditDefaultsOnly, Category = Coin)
	UCurveFloat* RotationCurve;

	/** The height this coin will fly. This will allow the location curve to use a simple 0 - 1 range */
	UPROPERTY(EditDefaultsOnly, Category = Coin)
	float FlipHeight;

	/** If the coin has been determined to land on heads (Player 1). This uses 
	GenerateCoinFlipWinner in CSKGameMode to set the value halfway during a flip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Coin)
	uint8 bLandingOnHeads : 1;

private:

	/** The timeline to play the coinflip */
	FTimeline FlipTimeline;

	/** If a side of the coin has been determined to land on */
	uint8 bWinnerDetermined : 1;

public:

	/** Get the location of the coin itself 
	(GetActorLocation does not return true location) */
	UFUNCTION(BlueprintPure, Category = Coin)
	FVector GetCoinLocation() const;
};
