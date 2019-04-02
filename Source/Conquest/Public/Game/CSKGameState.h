// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "CSKGameState.generated.h"

class ABoardManager;

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

	// Begin AGameStateBase Interface
	virtual void HandleBeginPlay() override;
	// End AGameStateBase Interface

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
	UFUNCTION(BlueprintPure, Category = CSK)
	ABoardManager* GetBoardManager(bool bErrorCheck = true) const;

private:

	/** The board of this match */
	UPROPERTY(VisibleInstanceOnly, Transient, Replicated, Category = CSK)
	ABoardManager* BoardManager;

public:

	/** Sets the state of the match */
	void SetMatchState(ECSKMatchState NewState);

public:

	/** Get the state of the match */
	FORCEINLINE ECSKMatchState GetMatchState() const { return MatchState; }

protected:

	/** Notify that match has entered pre match waiting phase */
	void NotifyWaitingForPlayers();

	/** Notify that the match has just begun */
	void NotifyMatchStart();

	/** Notify that the match has just finished */
	void NotifyMatchFinished();

	/** Notify that the players are leaving the match */
	void NotifyPlayersLeaving();

	/** Notify that the match as been abandoned */
	void NotifyMatchAbort();

private:

	/** Notify that match state has just been replicated */
	UFUNCTION()
	void OnRep_MatchState();

	/** Determines which state change notify to call */
	void HandleStateChange(ECSKMatchState NewState);

protected:

	/** The current state of the match */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_MatchState)
	ECSKMatchState MatchState;

	/** The last state we were running (client side) */
	UPROPERTY()
	ECSKMatchState PreviousState;
};
