// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "CSKGameState.generated.h"

class ABoardManager;
class ACastle;
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

public:

	/** Notify that a move request has been confirmed and is starting */
	void HandleMoveRequestConfirmed();

	/** Notify that the current move request has finished */
	void HandleMoveRequestFinished();

private:

	/** Handle movement request confirmation on every clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleMoveRequestConfirmed();

	/** Handle movement request confirmation on every clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_HandleMoveRequestFinished();

protected:

	/** How many rounds have been played */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = Stats)
	int32 RoundsPlayed;
};
