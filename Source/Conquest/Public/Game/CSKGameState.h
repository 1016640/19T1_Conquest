// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "CSKGameState.generated.h"

class ABoardManager;
class ACSKPlayerController;
class ATile;

/**
 * Tracks state of game and stats about the board
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	ACSKGameState();

protected:

	// Begin AGameStateBase Interface
	virtual void OnRep_ReplicatedHasBegunPlay() override;
	// End AGameStateBase Interface

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Called from game to set the board manager to be used for the match */
	void SetMatchBoardManager(ABoardManager* InBoardManager);

public:

	/** Get the games board manager */
	UFUNCTION(BlueprintPure, Category = "Board")
	ABoardManager* GetBoardManager(bool bErrorCheck = true) const;

private:

	/** The board of this match */
	UPROPERTY(VisibleInstanceOnly, Transient, Replicated, Category = CSK)
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

protected:

	/** ID of the player who goes first. This will be the player who won the coin flip */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	int32 StartingPlayerID;

protected:

	/** How many rounds have been played */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = Stats)
	int32 RoundsPlayed;

public:

	/** Notify that a move request has been confirmed and is starting */
	void HandleMoveRequestConfirmed() { }

	/** Notify that the current move request has finished */
	void HandleMoveRequestFinished() { }
};
