// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerState.h"
#include "CSKPlayerState.generated.h"

class ACastle;

/**
 * Tracks states and stats for a player
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ACSKPlayerState();

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Sets this players CSK player ID */
	void SetCSKPlayerID(int32 InPlayerID);

	/** Set this players castle */
	void SetCastle(ACastle* InCastle);

public:

	/** Get this players CSK player ID */
	FORCEINLINE int32 GetCSKPlayerID() const { return CSKPlayerID; }

	/** Get this players castle */
	FORCEINLINE ACastle* GetCastle() const { return Castle; }

protected:

	/** This players CSKPlayerID */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = CSK)
	int32 CSKPlayerID;

	/** This players castle */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = CSK)
	ACastle* Castle;

public:

	/** Gives this player both gold and mana (can be negative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void GiveResources(int32 InGold, int32 InMana);

	/** Overrides the amount of gold and mana this player has */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void SetResources(int32 InGold, int32 InMana);

	/** Gives gold to this player (can be negative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void AddGold(int32 Amount);

	/** Overrides the amount of gold this player has */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void SetGold(int32 Amount);

	/** Gives mana to this player (can be negative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void AddMana(int32 Amount);

	/** Overrides the amount of mana this player has */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Resources)
	void SetMana(int32 Amount);

public:

	/** If this player has required amount of gold */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool HasRequiredGold(int32 RequiredAmount) const;

	/** If this player has required amount of mana */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool HasRequiredMana(int32 RequiredAmount) const;

protected:

	/** The amount of gold this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	int32 Gold;

	/** The amount of mana this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	int32 Mana;

	// TODO: Rep spells here (only rep to owner though)
	// Not in this section, but how many spells we can use this round (also only reped to owner)

	/** This players assigned color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	FColor AssignedColor;

public:

	/** Increments the tiles we have traversed this round */
	void IncrementTilesTraversed();

	/** Resets the tiles traversed count for next round */
	void ResetTilesTraversed();

public:

	/** Get the amount of tiles this player has traversed this round */
	FORCEINLINE int32 GetTilesTraversedThisRound() const { return TilesTraversedThisRound; }

protected:

	/** The amount of tiles this player has moved this turn */
	UPROPERTY(Transient, Replicated)
	int32 TilesTraversedThisRound;

	/** The amount of tiles this player has moved in total */
	UPROPERTY(Transient)
	int32 TotalTilesTraversed;
};
