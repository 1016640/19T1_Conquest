// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "WinnerSequenceActor.generated.h"

class ACastle;
class UCurveFloat;

/** Delegate for when the sequence has finished */
DECLARE_DYNAMIC_DELEGATE(FWinnerSequenceFinished);

/**
 * Cosmetic actor that plays a cinematic for when the match has finished (in terms of a win condition has been met)
*/
UCLASS(abstract, notplaceable)
class CONQUEST_API AWinnerSequenceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AWinnerSequenceActor();

public:	
	
	// Begin AActor Interface
	virtual void BeginPlay() override;
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

	/** Sets all activation info */
	virtual void InitSequenceActor(ACSKPlayerState* InWinningPlayer, ECSKMatchWinCondition InWinCondition);

public:

	/** The player that won the match */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Sequence)
	ACSKPlayerState* WinningPlayer;

	/** The win condition the winning player met */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Sequence)
	ECSKMatchWinCondition WinCondition;

private:

	/** Constructs the timeline to use */
	virtual void ConstructTimeline();

	/** Handles update to dilation track */
	UFUNCTION()
	virtual void OnDilationInterpUpdated(float Value);

	/** Handles when sequence timeline has finished */
	UFUNCTION()
	virtual void OnTimelineFinished();

public:

	/** Event for when this sequence actor has finished. This only executes on the server */
	FWinnerSequenceFinished OnSequenceFinished;

protected:

	/** The interpolation track for the global time dilation. This should both 
	start and end at 1 (though will be auto restored once timeline is done) */
	UPROPERTY(EditDefaultsOnly, Category = Coin)
	UCurveFloat* DilationCurve;

private:

	/** The timeline to play during the sequence. When this time
	line ends is when the sequence is considered to have finished */
	FTimeline SequenceTimeline;
};

/**
 * Winner sequence actor for when the a player has destroyed their opponents castle
*/
UCLASS(abstract)
class CONQUEST_API ACastleDestroyedSequenceActor : public AWinnerSequenceActor
{
	GENERATED_BODY()

public:

	// Begin AWinnerSequenceActor Interface
	virtual void InitSequenceActor(ACSKPlayerState* InWinningPlayer, ECSKMatchWinCondition InWinCondition) override;
	// End AWinnerSequenceActor

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** The opponents castle (This is the castle that has been destroyed) */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Sequence)
	ACastle* OpponentsCastle;

	/** The random stream to use for aethetic reasons. This is synced amongst
	all instances of this actor for all clients (but not number of uses) */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Sequence)
	FRandomStream RandomStream;

protected:

	/** Generates a random location around the opponents castle within given
	bounds. This will use the random stream that was generated originally */
	UFUNCTION(BlueprintPure, Category = Sequence, meta = (BlueprintProtected = "true"))
	FVector GetPointAroundCastle(const FVector& Bounds) const;
};