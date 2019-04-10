// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "CSKGameState.generated.h"

class ABoardManager;
class ACastle;
class ACSKPlayerController;
class ACSKPlayerState;
class ACSKGameMode;
class ATile;
class ATower;
class UTowerConstructionData;

/** Delegate for when the round state changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCSKRoundStateChanged, ECSKRoundState, NewState);

/**
 * Tracks state of game and stats about the board
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	ACSKGameState();

public:

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin AGameStateBase Interface
	virtual void OnRep_ReplicatedHasBegunPlay() override;
	// End AGameStateBase Interface

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Do not call this externally. This is used by the game mode to set the board to use */
	void SetMatchBoardManager(ABoardManager* InBoardManager);

public:

	/** Get the games board manager */
	UFUNCTION(BlueprintPure, Category = "Board")
	ABoardManager* GetBoardManager(bool bErrorCheck = true) const;

private:

	/** The board of this match */
	UPROPERTY(Transient, Replicated)
	ABoardManager* BoardManager;

public:

	/** Sets the state of the match */
	void SetMatchState(ECSKMatchState NewState);

	/** Sets the state of the round */
	void SetRoundState(ECSKRoundState NewState);

public:

	/** Get the state of the match */
	FORCEINLINE ECSKMatchState GetMatchState() const { return MatchState; }

	/** Get the state of the round */
	FORCEINLINE ECSKRoundState GetRoundState() const { return RoundState; }

	/** Get if the match is active */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsMatchInProgress() const;

	/** Get if an action phase is active */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsActionPhaseActive() const;

protected:

	/** Match state notifies */
	void NotifyWaitingForPlayers();
	void NotifyPerformCoinFlip();
	void NotifyMatchStart();
	void NotifyMatchFinished();
	void NotifyPlayersLeaving();
	void NotifyMatchAbort();

	/** Round state notifies */
	void NotifyCollectionPhaseStart();
	void NotifyFirstCollectionPhaseStart();
	void NotifySecondCollectionPhaseStart();
	void NotifyEndRoundPhaseStart();

private:

	/** Notify that match state has just been replicated */
	UFUNCTION()
	void OnRep_MatchState();

	/** Determines which match state change notify to call */
	void HandleMatchStateChange(ECSKMatchState NewState);

	/** Notify that round state has just been replicated */
	UFUNCTION()
	void OnRep_RoundState();

	/** Determines which round state change notify to call */
	void HandleRoundStateChange(ECSKRoundState NewState);

public:

	/** Event called when the round state has changed */
	UPROPERTY(BlueprintAssignable, Category = CSK)
	FCSKRoundStateChanged OnRoundStateChanged;

protected:

	/** The current state of the match */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_MatchState)
	ECSKMatchState MatchState;

	/** The last match state we were running (client side) */
	UPROPERTY()
	ECSKMatchState PreviousMatchState;

	/** During match, what phase of the round we are up to */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_RoundState)
	ECSKRoundState RoundState;

	/** The last round phase we were running (client side) */
	UPROPERTY()
	ECSKRoundState PreviousRoundState;

public:

	/** Get if action phase is timed */
	UFUNCTION(BlueprintPure, Category = Rules)
	bool IsActionPhaseTimed() const { return ActionPhaseTimeRemaining != -1.f; }

	/** Get the amount of instances of given type of tower active on the board */
	UFUNCTION(BlueprintPure, Category = Rules)
	int32 GetTowerInstanceCount(TSubclassOf<ATower> Tower) const;

protected:

	/** Helper function for checking if action phase timer should count down */
	FORCEINLINE bool ShouldCountdownActionPhase() const
	{
		if (IsActionPhaseActive() && IsActionPhaseTimed())
		{
			return !bFreezeActionPhaseTimer;
		}

		return false;
	}

	/** Helper function for enabling/disabling action phase timer */
	FORCEINLINE void SetFreezeActionPhaseTimer(bool bEnable)
	{
		bFreezeActionPhaseTimer = bEnable;
		SetActorTickEnabled(!bEnable);
	}

	/** Helper function for adding bonus time to action phase timer */
	void AddBonusActionPhaseTime();

private:

	/** Resets action phase variables */
	void ResetActionPhaseProperties();

protected:

	/** ID of the player whose action phase it is */
	UPROPERTY(Transient, Replicated)
	int32 ActivePhasePlayerID;

	/** Time remaining in this action phase */
	UPROPERTY(Transient, Replicated)
	float ActionPhaseTimeRemaining;

	/** If action phase timer has been frozen */
	UPROPERTY(Transient)
	uint32 bFreezeActionPhaseTimer : 1;

	/** Lookup table for how many instances of a certain tower exists on the board */
	UPROPERTY(Transient)
	TMap<TSubclassOf<ATower>, int32> TowerInstanceTable;

public:

	/** Notify that a move request has been confirmed and is starting */
	void HandleMoveRequestConfirmed();

	/** Notify that the current move request has finished */
	void HandleMoveRequestFinished();

	/** Notify that a new tower has been placed on the map */
	void HandleBuildRequestConfirmed(ATile* TargetTile);

	/** Notify that the current build request has finished */
	void HandleBuildRequestFinished(ATower* NewTower);

private:

	/** Handle movement request confirmation client side */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleMoveRequestConfirmed();

	/** Handle movement request finished client side */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleMoveRequestFinished();

	/** Handle build request confirmation client side */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleBuildRequestConfirmed(ATile* TargetTile);

	/** Handle build request finished client side */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleBuildRequestFinished(ATower* NewTower);

public:

	/** If given player has moved the required amount of tiles this turn */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasPlayerMovedRequiredTiles(const ACSKPlayerController* Controller) const;

	/** If given player can build or destroy the given tower */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool CanPlayerBuildTower(const ACSKPlayerController* Controller, TSubclassOf<UTowerConstructionData> TowerTemplate, bool bOrDestroy = false) const;

	/** If given player can build or destroy anymore towers this turn */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool CanPlayerBuildMoreTowers(const ACSKPlayerController* Controller, bool bOrDestroy = false) const;

	/** Get all towers that can be built this match */
	FORCEINLINE const TArray<TSubclassOf<UTowerConstructionData>>& GetAvailableTowers() const { return AvailableTowers; }

protected:

	/** Updates the rules variables by cloning rules establish by game mode */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = CSK)
	void UpdateRules();

	/** Helper function for checking if given player can build or destroy given tower */
	// TODO: Implement OrDestroy functionality
	bool CanPlayerBuildTower(const ACSKPlayerState* PlayerState, TSubclassOf<UTowerConstructionData> TowerTemplate, bool bOrDestroy) const;
	
protected:

	/** Cached action phase timer used to reset action phase time each round */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	float ActionPhaseTime;

	/** The max number of NORMAL towers players are allowed to build */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MaxNumTowers;

	/** The max number of duplicated NORMAL towers a player can have built at once */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MaxNumDuplicatedTowers;

	/** The max number of LEGENDARY towers a player can have built at once */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MaxNumLegendaryTowers;

	/** The max range from the players castle they can build from */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MaxBuildRange;

	/** The towers supported for this match */
	// TODO: See CSKGameMode.h (ln 412) for a TODO
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	TArray<TSubclassOf<UTowerConstructionData>> AvailableTowers;

	/** The minimum amount of tiles a player must move each action phase */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MinTileMovements;

	/** The maximum amount of tiles a player can move each action round */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	int32 MaxTileMovements;

protected:

	/** How many rounds have been played */
	UPROPERTY(BlueprintReadOnly, Category = Stats)
	int32 RoundsPlayed;
};
