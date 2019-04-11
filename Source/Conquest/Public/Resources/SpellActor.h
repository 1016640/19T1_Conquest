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
 * in as a spell is being used then immediately destroyed afterwards
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
	void SetActivationInfo(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, ATile* InTargetedTile);

	/** Notify from game mode to begin execution of this spell */
	void BeginExecution();

protected:

	/** Actives this spell, any effects the spell may have should be applied on this event. This event runs on the server,
	Once all events have concluded, FinishSpell() should be called to end activation of this spell */
	UFUNCTION(BlueprintNativeEvent, Category = Spells)
	void OnActivateSpell();

	/** Finishes casting this spell. Only runs on the server and notifies game mode */
	UFUNCTION(BlueprintCallable, Category = Spells)
	void FinishSpell();

protected:

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
	uint8 bIsRunning : 1;
};
