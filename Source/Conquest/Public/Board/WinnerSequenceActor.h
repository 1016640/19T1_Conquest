// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "WinnerSequenceActor.generated.h"

class ACastle;
class UCurveFloat;
class UCurveVector;

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

protected:

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

	/** The timeline to play during the sequence. When this time
	line ends is when the sequence is considered to have finished */
	FTimeline SequenceTimeline;
};

/**
 * Winner sequence actor for when a player has reached their opponents castle. This
 * sequence actor has the winning players castle disappear into their opponents portal
*/
UCLASS(abstract)
class CONQUEST_API APortalReachedSequenceActor : public AWinnerSequenceActor
{
	GENERATED_BODY()

public:

	// Begin AActor Interface
	virtual void BeginPlay() override;
	// End AActor Interface

protected:

	// Begin AWinnerSequenceActor Interface
	virtual void ConstructTimeline() override;
	virtual void OnTimelineFinished() override;
	// End AWinnerSequenceActor Interface

private:

	/** The rotation of the castles mesh when we first started interpolation */
	FRotator InitialRotation;
	
protected:

	/** Handles update to rotation track */
	UFUNCTION()
	void OnRotationInterpUpdated(float Value);

	/** Handles update to scaling track */
	UFUNCTION()
	void OnScaleInterpUpdated(FVector Value);

protected:

	/** The interpolation track for the rotation of the winning players castle. This will
	only apply to the castles Yaw Rotation (With the value expected to be between 0 and 1) */
	UPROPERTY(EditDefaultsOnly, Category = Sequence)
	UCurveFloat* RotationCurve;

	/** The interpolation track for the scaling of the winning players castle.
	This curve is expected to be in world space and to provide world space coords */
	UPROPERTY(EditDefaultsOnly, Category = Sequence)
	UCurveVector* ScaleCurve;
};

/**
 * Winner sequence actor for when a player has destroyed their opponents castle. This
 * sequence actor has the opponents castle being blown up with multiple explosions
*/
UCLASS(abstract)
class CONQUEST_API ACastleDestroyedSequenceActor : public AWinnerSequenceActor
{
	GENERATED_BODY()

public:

	ACastleDestroyedSequenceActor();

public:

	// Begin AWinnerSequenceActor Interface
	virtual void InitSequenceActor(ACSKPlayerState* InWinningPlayer, ECSKMatchWinCondition InWinCondition) override;
	// End AWinnerSequenceActor Interface

	// Begin AActor Interface
	virtual void BeginPlay() override;
	// End AActor Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
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

	/** The min interval range for spawning an explosion. The interval is calculated by RandomStream */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sequence, meta = (ClampMin=0.1))
	float MinDelayInterval;

	/** The max interval range for spawning an explosion. The interval is calculated by RandomStream */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sequence, meta = (ClampMin = 0.1))
	float MaxDelayInterval;

protected:

	/** Executed when the random delay has surpassed. This is also called during begin play */
	UFUNCTION(BlueprintImplementableEvent, Category = Sequence, meta = (DisplayName = "On Interval Event"))
	void BP_OnIntervalEvent();

private:

	/** Notify that the random delay interval has finished. This is also called during begin
	play to act as an initial 'burst' and to start the loop with a random calculated delay */
	void OnIntervalDelayFinished();

private:

	/** Handle to the interval event */
	FTimerHandle Handle_IntervalEvent;

protected:

	/** Generates a random location around the opponents castle within given
	bounds. This will use the random stream that was generated originally */
	UFUNCTION(BlueprintPure, Category = Sequence, meta = (BlueprintProtected = "true"))
	FVector GetPointAroundCastle(const FVector& Bounds) const;
};