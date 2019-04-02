// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "CSKGameMode.generated.h"

class ACastle;
class ACastleAIController;
class ACSKPlayerController;

/** The current state of the session (not the match itself). 
This works similar to how AGameMode works (see GameMode.h) */
enum class ECSKSessionState : uint8
{
	/** We are entering the map in which to play */
	EnteringMap,

	/** We are waiting for all clients to be ready (actors are already ticking) */
	PreMatchWait,

	/** The match is in progress */
	InProgress,

	/** Match has finished via a win or lose condition. We are now waiting before exiting (actors are still ticking) */
	PostMatchWait,

	/** We are leaving the map and returning to lobby */
	LeavingMap,

	/** Match was aborted due to unseen circumstances */
	Aborted
};

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

	// Begin AGameModeBase Interface
	//virtual void InitGameState() override;
	//virtual void StartPlay() override;
	//virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	virtual bool HasMatchStarted() const override;	
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes)
	TSubclassOf<ACastle> Player1CastleClass;

	/** The class to use to spawn the second players castle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes)
	TSubclassOf<ACastle> Player2CastleClass;	

	/** The castle controller to use (if none is specified, we used the default AI controller in castle class) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Classes)
	TSubclassOf<ACastleAIController> CastleAIControllerClass;
};
