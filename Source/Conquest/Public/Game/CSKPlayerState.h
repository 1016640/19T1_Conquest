// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerState.h"
#include "CSKPlayerState.generated.h"

class ACastle;
class ACSKPlayerController;
class ATower;
class USpellCard;

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

	/** Get this states owner as a CSKPlayerController */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerController* GetCSKPlayerController() const;

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

	/** Adds a tower to the list of towers this player owns */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Board)
	void AddTower(ATower* InTower);
	
	/** Removes a tower from the list of towers this player owns */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Board)
	void RemoveTower(ATower* InTower);

	/** Retrieves a spell card from the spell deck and places in the players hand */
	TSubclassOf<USpellCard> PickupCardFromDeck();

	/** Resets the spell deck by copying then shuffling given spells */
	void ResetSpellDeck(const TArray<TSubclassOf<USpellCard>>& Spells);

public:

	/** If this player has required amount of gold */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool HasRequiredGold(int32 RequiredAmount) const;

	/** If this player has required amount of mana */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool HasRequiredMana(int32 RequiredAmount) const;

	/** Get how many of given type of tower this player owns */
	UFUNCTION(BlueprintPure, Category = Resources)
	int32 GetNumOwnedTowerDuplicates(TSubclassOf<ATower> Tower) const;

	/** Get if this player is able to cast another spell */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool CanCastAnotherSpell() const { return HasInfiniteSpellUses() || MaxNumSpellUses < SpellsCastThisRound; }

	/** Get if this player has infinite spell uses */
	UFUNCTION(BlueprintPure, Category = Resources)
	bool HasInfiniteSpellUses() const { return MaxNumSpellUses == -1; }

public:

	/** Get this players color */
	FORCEINLINE const FColor& GetAssignedColor() const { return AssignedColor; }

	/** Get the amount of gold this player has */
	FORCEINLINE int32 GetGold() const { return Gold; }

	/** Get the amount of mana this player has */
	FORCEINLINE int32 GetMana() const { return Mana; }

	/** Get the towers this player owns */
	FORCEINLINE const TArray<ATower*> GetOwnedTowers() const { return OwnedTowers; }

	/** Get the number of NORMAL towers this player owns */
	UFUNCTION(BlueprintPure, Category = Board)
	int32 GetNumNormalTowersOwned() const { return OwnedTowers.Num() - CachedNumLegendaryTowers; }

	/** Get the number of LEGENDARY towers this player owns */
	UFUNCTION(BlueprintPure, Category = Board)
	int32 GetNumLegendaryTowersOwned() const { return CachedNumLegendaryTowers; }

	/** Get the total number of towers this player owns */
	UFUNCTION(BlueprintPure, Category = Board)
	int32 GetNumTowersOwned() const { return OwnedTowers.Num(); }

	/** Get spells in this players deck */
	FORCEINLINE const TArray<TSubclassOf<USpellCard>>& GetSpellCardDeck() const { return SpellCardDeck; }
	
	/** Get spells in this players hand */
	FORCEINLINE const TArray<TSubclassOf<USpellCard>>& GetSpellCardsInHand() const { return SpellCardsInHand; }

	/** Get how many spells cards this player has in their hand */
	UFUNCTION(BlueprintPure, Category = Spells)
	int32 GetNumSpellsInHand() const { return SpellCardsInHand.Num(); }

	/** Get if this player needs to reshuffle and reset thier spell
	deck. This is when all spell cards are in the discard pile */
	bool NeedsSpellDeckReshuffle() const;

protected:

	/** Notify that owned towers has been replicated */
	UFUNCTION()
	void OnRep_OwnedTowers();

private:

	/** Updates our cached tower numbers based on current state of owned towers */
	void UpdateTowerCounts();

protected:

	/** This players assigned color */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Resources)
	FColor AssignedColor;

	/** The amount of gold this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Resources)
	int32 Gold;

	/** The amount of mana this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Resources)
	int32 Mana;

	/** The towers this player owns */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_OwnedTowers, Category = Resources)
	TArray<ATower*> OwnedTowers;

	/** Cached count of how many LEGENDARY towers this player owns */
	int32 CachedNumLegendaryTowers;

	/** Cached map for counting how many of a certain type of any tower this players owns */
	TMap<TSubclassOf<ATower>, int32> CachedUniqueTowerCount;

	// TODO: Rep spells here (only rep to owner though)
	// Not in this section, but how many spells we can use this round (also only reped to owner)

	// TODO: Either replicate arrays (COND_OwnerOnly) or use RPC (via PlayerController) (maybe excluding DiscardPile)

	/** The spells cards in the spell deck. This only exists on the server */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Resources)
	TArray<TSubclassOf<USpellCard>> SpellCardDeck;

	/** The spells in the players hand. This only exists on the server and the owners client */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = Resources)
	TArray<TSubclassOf<USpellCard>> SpellCardsInHand;

	/** The spells cards in the discard pile. This only exists on the server and the owners client */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Resources)
	TArray<TSubclassOf<USpellCard>> DiscardPile;
	public:
	/** The number of spells this player is allowed to use. Set value to -1 for infinite spell uses */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = Resources)
	int32 MaxNumSpellUses;

public:

	/** Increments the tiles we have traversed this round */
	void IncrementTilesTraversed();

	/** Resets the tiles traversed count for next round */
	void ResetTilesTraversed();

	/** Increments the spells we have cast this round */
	void IncrementSpellsCast();

	/** Resets the spells cast count for next round */
	void ResetSpellsCast();

public:

	/** Get the amount of tiles this player has traversed this round */
	FORCEINLINE int32 GetTilesTraversedThisRound() const { return TilesTraversedThisRound; }

	/** Get the amount of spells this player has cast this round */
	FORCEINLINE int32 GetSpellsCastThisRound() const { return SpellsCastThisRound; }

protected:

	/** The amount of tiles this player has moved this turn */
	UPROPERTY(Transient, Replicated)
	int32 TilesTraversedThisRound;

	/** The amount of tiles this player has moved in total */
	UPROPERTY(Transient)
	int32 TotalTilesTraversed;

	/** The amount of NORMAL towers this player has built in total */
	UPROPERTY(Transient)
	int32 TotalTowersBuilt;

	/** The amount of LEGENDARY towers this player has built in total */
	UPROPERTY(Transient)
	int32 TotalLegendaryTowersBuilt;

	/** The amount of spells this player has cast this turn */
	UPROPERTY(Transient, Replicated)
	int32 SpellsCastThisRound;

	/** The total amount of action phase spells this player has cast */
	UPROPERTY(Transient)
	int32 TotalSpellsCast;

	/** The total amount of quick effect spells this player has cast */
	UPROPERTY(Transient)
	int32 TotalQuickEffectSpellsCast;
};
