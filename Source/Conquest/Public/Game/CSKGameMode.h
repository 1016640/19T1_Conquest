// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "CSKGameMode.generated.h"

class ACastle;
class ACastleAIController;
class ACSKPlayerController;

/**
 * Manages and handles events present in CSK
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	ACSKGameMode();

public:

	virtual void Tick(float DeltaTime) override;

	// Begin AGameModeBase Interface
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	//virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	//virtual bool HasMatchStarted() const override;	

	//virtual void StartToLeaveMap() override;
	// End AGameModeBase Interface

private:

	/** Spawns default castle for given controller. Get the AI controller possessing the newly spawned castle */
	ACastleAIController* SpawnDefaultCastleFor(ACSKPlayerController* NewPlayer) const;

	/** Spawns a castle at portal corresponding with player ID */
	ACastle* SpawnCastleAtPortal(int32 PlayerID, TSubclassOf<ACastle> Class) const;

	/** Spawns the AI controller for given castle. Will auto possess the castle if successful */
	ACastleAIController* SpawnCastleControllerFor(ACastle* Castle) const;

public:

	/** Get if controller is either player 1 or player 2 (-1 if niether) */
	UFUNCTION(BlueprintPure, Category = CSK)
	int32 GetControllerAsPlayerID(AController* Controller) const;

protected:

	/** The first player that joined the game (should be server player is listen server) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = CSK)
	ACSKPlayerController* Player1PC;

	/** The second player that joined (should always be a remote client) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = CSK)
	ACSKPlayerController* Player2PC;

	/** The class to use to spawn the first players castle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes, meta = (DisplayName = "Player 1 Castle Class"))
	TSubclassOf<ACastle> Player1CastleClass;

	/** The class to use to spawn the second players castle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes, meta = (DisplayName = "Player 2 Castle Class"))
	TSubclassOf<ACastle> Player2CastleClass;	

	/** The castle controller to use (if none is specified, we used the default AI controller in castle class) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes)
	TSubclassOf<ACastleAIController> CastleAIControllerClass;

public:

	/** Enters the given state */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EnterMatchState(ECSKMatchState NewState);

	/** Starts the match only if allowed */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void StartMatch();

	/** Ends the match only if allowed */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EndMatch();

	/** Forcefully ends the match with no winner decided */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void AbortMatch();

public:

	/** Get the current state of the match */
	FORCEINLINE ECSKMatchState GetMatchState() const { return MatchState; }

	/** Get if we can start the match at this point */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool CanStartMatch() const;
	virtual bool CanStartMatch_Implementation() const;

	/** Get if we can finish the match at this point */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool CanEndMatch() const;
	virtual bool CanEndMatch_Implementation() const;

	/** Get if the match is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsMatchInProgress() const;

	/** Get if the match has ended */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasMatchFinished() const;

protected:

	/** Notify that we are waiting for players to finish joining */
	void OnWaitingForPlayers();

	/** Notify that the match can now start */
	void OnMatchStart();

	/** Notify that the match has come to a conclusion */
	void OnMatchFinished();

	/** Notify that players are now leaving the session */
	void OnPlayersLeaving();

	/** Notify that the game has been aborted */
	void OnMatchAbort();

private:

	/** Determines the state change event to call based on previous and new state */
	void HandleStateChange(ECSKMatchState OldState, ECSKMatchState NewState);

protected:

	/** The current state of the session */
	UPROPERTY(BlueprintReadOnly, Transient)
	ECSKMatchState MatchState;

protected:

	/** Notify that a client has disconnected */
	void OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver);
};
