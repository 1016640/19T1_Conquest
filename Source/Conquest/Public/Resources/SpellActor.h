// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "SpellActor.generated.h"

class ACSKPlayerState;
class ATile;
class USpell;

/**
 * An actor that performs the actions of a spell. This actor gets spawned
 * in as a spell is being used then immediately destroyed afterwards. When
 * executing the action, CheckSpellIsCancelled should be called before commiting
 * to different stages of an action, with the action only progressing if it is cancelled.
 */
UCLASS(abstract, notplaceable)
class CONQUEST_API ASpellActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ASpellActor();

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Sets all activation info */
	void InitSpellActor(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, ATile* InTargetedTile);

	/** Notify from game mode to begin execution of this spell */
	void BeginExecution();

	/** Notify from game mode to cancel execution of this spell */
	void CancelExecution();

protected:

	/** Actives this spell, any effects the spell may have should be applied on this event. This event runs on the server,
	Once all events have concluded, FinishSpell() should be called to end activation of this spell */
	UFUNCTION(BlueprintNativeEvent, Category = Spells)
	void OnActivateSpell();

	/** Finishes casting this spell. Only runs on the server and notifies game mode.
	This does not need to be called if the spell was cancelled */
	UFUNCTION(BlueprintCallable, Category = Spells)
	void FinishSpell();

	/** Checks if this spell has been cancelled, calling fisish spell if the case.
	Get if the spell was indeed cancelled and the spell has now finished */
	UFUNCTION(BlueprintCallable, Category = Spells)
	bool CheckSpellIsCancelled();

public:

	/** The player currently executing this event */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	ACSKPlayerState* CastingPlayer;

	/** The spell the player is casting (and we are enacting) */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	USpell* CastingSpell;

	/** The amount paid to activate this spell */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	int32 ActivationCost;

	/** The tile the player targeted this spell with */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	ATile* TargetedTile;

private:

	/** If this spell is running */
	uint8 bRunning : 1;

	/** If this spell has been cancelled */
	uint8 bCancelled : 1;
};
