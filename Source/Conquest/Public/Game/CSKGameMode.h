// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "CSKGameMode.generated.h"

class ACastle;
class ACastleAIController;
class ACSKPlayerController;
class ATile;

/**
 * Manages and handles events present in CSK
 */
UCLASS(config=Game, ClassGroup = (CSK))
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

	virtual bool HasMatchStarted() const override;	

	//virtual void StartToLeaveMap() override;
	// End AGameModeBase Interface

protected:

	// Begin UObject Interface
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface

public:

	/** Checks if match in current state is valid and can continue */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsMatchValid() const;

private:

	/** Spawns default castle for given controller. Get the AI controller possessing the newly spawned castle */
	ACastleAIController* SpawnDefaultCastleFor(ACSKPlayerController* NewPlayer) const;

	/** Spawns a castle at portal corresponding with player ID */
	ACastle* SpawnCastleAtPortal(int32 PlayerID, TSubclassOf<ACastle> Class) const;

	/** Spawns the AI controller for given castle. Will auto possess the castle if successful */
	ACastleAIController* SpawnCastleControllerFor(ACastle* Castle) const;

public:

	/** Get player ones controller */
	FORCEINLINE ACSKPlayerController* GetPlayer1Controller() const { return Player1PC; }

	/** Get player twos controller */
	FORCEINLINE ACSKPlayerController* GetPlayer2Controller() const { return Player2PC; }

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

	/** Enters the given state if not set already */
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

	/** Get if we should start the match */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool ShouldStartMatch() const;
	virtual bool ShouldStartMatch_Implementation() const;

	/** Get if we should finish the match */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool ShouldEndMatch() const;
	virtual bool ShouldEndMatch_Implementation() const;

	/** Get if the match is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsMatchInProgress() const;

	/** Get if the match has ended */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasMatchFinished() const;

protected:

	/** Notify that we can run anything that can start before the match */
	virtual void OnStartWaitingPreMatch();

	/** Notify that the match can now start */
	virtual void OnMatchStart();

	/** Notify that the match has come to a conclusion. We should
	now have a small cooldown or immediately exit the game */
	virtual void OnMatchFinished();

	/** Notify that post match stage has concluded and players can now return to lobby */
	virtual void OnFinishedWaitingPostMatch();

	/** Notify that the game has been abandoned */
	virtual void OnMatchAbort();

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

public:

	/** Performs a coin flip function, should return true if heads, false if tails */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = CSK)
	bool CoinFlip() const;

public:

	/** Resets both players resources based on default resource settings */
	void ResetResourcesForPlayers();
	
	/** Updates both players resources based on default 
	resource collection and tower resource bonuses */
	void CollectResourcesForPlayers();

private:

	/** Resets the resources player has to defaults.
	Player ID is passed for logging purposes only, it should not be used to determine which player is being updated */
	void ResetPlayerResources(ACSKPlayerController* Controller, int32 PlayerID);

	/** Gives player default resources, as well as any additional resources provided by towers.
	Player ID is passed for logging purposes only, it should not be used to determine which player is being updated */
	void UpdatePlayerResources(ACSKPlayerController* Controller, int32 PlayerID);

public:

	/** Clamps value based on max gold allowed */
	UFUNCTION(BlueprintPure, Category = Resources)
	int32 ClampGoldToLimit(int32 Gold) const { return FMath::Clamp(Gold, 0, MaxGold); }
	
	/** Clamps value based on max mana allowed */
	UFUNCTION(BlueprintPure, Category = Resources)
	int32 ClampManaToLimit(int32 Mana) const { return FMath::Clamp(Mana, 0, MaxMana); }

protected:

	/** The amount of gold each player starts with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 StartingGold;

	/** How much gold is given to each player during the collection phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 CollectionPhaseGold;

	/** The max amount of gold a player can hold at a time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 MaxGold;

	/** The amount of mana each player starts with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 StartingMana;

	/** How much mana is given to each player during the collection phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 CollectionPhaseMana;

	/** The max amount of mana a player can hold at a time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 MaxMana;

public:

	/** Get if given value is within the limit of tiles that can be traversed each round */
	FORCEINLINE bool IsCountWithinTileTravelLimits(int32 Count) const { return (MinTileMovements <= Count && Count <= MaxTileMovements); }

	/** Get the time an action phase lasts */
	UFUNCTION(BlueprintPure, Category = Rules)
	float GetActionPhaseTime() const { return ActionPhaseTime; }

	/** Get the max amount of tiles that can be traversed per action phase */
	UFUNCTION(BlueprintPure, Category = Rules)
	int32 GetMaxTileMovementsPerTurn() const { return MaxTileMovements; }

protected:

	/** The amount of time (in seconds) an action phase lasts before forcing exit (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	float ActionPhaseTime;

	/** The min amount of tiles a player can move per action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 1))
	int32 MinTileMovements;

	/** The max amount of tiles a player can move per action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 1))
	int32 MaxTileMovements;

public:

	/** Will have the given controllers castle travel to requested tile */
	void MovePlayersCastleTo(ACSKPlayerController* Controller, ATile* Tile) const;

private:

	/** Get if player is allowed to travel from starting tile to goal tile */
	bool CanPlayerMoveToTile(ACSKPlayerController* Controller, ATile* From, ATile* To) const;
};
