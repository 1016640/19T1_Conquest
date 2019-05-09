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
	void InitSpellActor(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, int32 InAdditionalMana, ATile* InTargetedTile);

	/** Notify from game mode to begin execution of this spell */
	void BeginExecution(bool bIsPrimeSpell);

	/** Notify from game mode to cancel execution of this spell */
	void CancelExecution();

protected:

	/** Actives this spell, any effects the spell may have should be applied on this event. This event runs on the server,
	Once all events have concluded, FinishSpell() should be called to end activation of this spell */
	UFUNCTION(BlueprintNativeEvent, Category = Spells)
	void OnActivateSpell(bool bIsPrimeSpell);

	/** Finishes casting this spell. Only runs on the server and notifies game mode.
	This does not need to be called if the spell was cancelled */
	UFUNCTION(BlueprintCallable, Category = Spells)
	void FinishSpell();

	/** Checks if this spell has been cancelled, calling fisish spell if the case.
	Get if the spell was indeed cancelled and the spell has now finished */
	UFUNCTION(BlueprintCallable, Category = Spells)
	bool CheckSpellIsCancelled();

public:

	/** If this spell is a primary or secondary spell */
	UFUNCTION(BlueprintPure, Category = Spells)
	bool IsPrimarySpell() const { return bIsPrimarySpell; }

	/** If this spell is allowed to continue execution */
	UFUNCTION(BlueprintPure, Category = Spells)
	bool CanSpellContinueExecution() const { return bRunning && !bCancelled; }

public:

	/** The player currently executing this event */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	ACSKPlayerState* CastingPlayer;

	/** The spell the player is casting (and we are enacting) */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	USpell* CastingSpell;

	/** The amount paid to activate this spell (discount applied) */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	int32 ActivationCost;

	/** The additional mana provided by the caster */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	int32 AdditionalMana;

	/** The tile the player targeted this spell with */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Activation)
	ATile* TargetedTile;

private:

	/** If this spell is the primary spell being cast. If this is false,
	it means this spell actor was spawned from an already active spell */
	uint8 bIsPrimarySpell : 1;

	/** If this spell is running */
	uint8 bRunning : 1;

	/** If this spell has been cancelled */
	uint8 bCancelled : 1;

protected:

	/** Enables input from our casters player controller. On Tile Selected
	will only be called on the server, all other inputs on the client */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Spells, meta = (BlueprintProtected = "true"))
	void BindPlayerInput();

	/** Disables input from our casters player controller. Will automatically
	be called when ending execution while input has been bound */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Spells, meta = (BlueprintProtected = "true"))
	void UnbindPlayerInput();

	/** Enables a custom timer (via CSKGameState) that will finished after given duration.
	This has the possibility of failing, this function will not handle re-trying */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Spells, meta = (BlueprintProtected = "true"))
	bool SetCustomTimer(int32 Duration);

	/** Disables the custom timer (if set). Will automatically be called
	when ending execution or the already bound timer has executed */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Spells, meta = (BlueprintProtected = "true"))
	void ClearCustomTimer();

	/** If the player is allowed to select the given tile for spell.
	This can run on both the owning players client and the server */
	UFUNCTION(BlueprintImplementableEvent, Category = Spells, meta = (DisplayName = "Can Select Tile for Spell"))
	bool BP_CanSelectTileForSpell(ATile* TargetTile);

	/** Called when player has selected the given tile for spell. This only
	executes on the server and when CanSelectTileForSpell has returned true */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Spells, meta = (DisplayName = "On Tile Selected for Spell"))
	void BP_OnTileSelectedForSpell(ATile* TargetTile);

	/** Called when custom timer has finished (or has been interrupted) */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Spells, meta = (DisplayName = "On Spell Custom Timer Finished"))
	void BP_OnSpellCustomTimerFinished(bool bFinished);

private:

	/** Binds player input client side */
	UFUNCTION(Client, Reliable)
	void Client_BindPlayerInput(bool bBind);

	/** Callback from timer when duration has elapsed */
	UFUNCTION()
	void OnCustomTimerFinished(bool bWasSkipped);

private:

	/** If player input has been bound */
	uint8 bIsInputBound : 1;

	/** If custom timer callback has been bound */
	uint8 bIsTimerBound : 1;

protected:

	/** Casts a sub spell with this spells caster also being the caster. Takes in an optional cost override, 
	with any value equal or greater than zero being a valid override. This spell could potentially override 
	this spells input bindings (same if spawning multiple), be sure to keep this in mind.	
	Will return a valid spell actor if spell cast was successfull, null if not.*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Spells, meta = (BlueprintProtected = "true"))
	ASpellActor* CastSubSpell(TSubclassOf<USpell> InSpell, ATile* InTargetTile, int32 InAdditionalMana = 0, int32 InOverrideCost = -1);

	/** Notify that one of the sub spells we cast has finished execution. This is only called on the server */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Spells, meta = (DisplayName = "On Sub Spell Finished"))
	void BP_OnSubSpellFinished(ASpellActor* FinishedSpell);

private:

	/** Callback that a sub spell has finished. This spell should belong to us */
	UFUNCTION()
	void OnSubSpellFinished(ASpellActor* FinishedSpell);

private:

	/** The sub spells we have cast. Only valid if a primary spell */
	UPROPERTY(VisibleInstanceOnly, Transient, AdvancedDisplay, Category = Spells)
	TSet<ASpellActor*> ActiveSubSpells;
};
