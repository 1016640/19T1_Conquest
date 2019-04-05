// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "BoardTypes.h"
#include "CSKGameMode.generated.h"

class ACastle;
class ACastleAIController;
class ACSKPlayerController;
class ATile;

using FCSKPlayerControllerArray = TArray<ACSKPlayerController*, TFixedAllocator<CSK_MAX_NUM_PLAYERS>>;

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
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerController* GetPlayer1Controller() const { return Players[0]; }

	/** Get player twos controller */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerController* GetPlayer2Controller() const { return Players[1]; }

	/** Get both players in the array*/
	const FCSKPlayerControllerArray& GetPlayers() const { return Players; }

	/** Based on given player ID, get the opposing players controller.
	E.G. Passing ID for player 1 (0) will return player 2s controller */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerController* GetOpposingPlayersController(int32 PlayerID) const;

private:

	/** Helper function for setting a player to ID */
	void SetPlayerWithID(ACSKPlayerController* Controller, int32 PlayerID);

protected:

	/** Array containing both players in order */
	FCSKPlayerControllerArray Players;

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

	/** Enters the given match state if not set already */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EnterMatchState(ECSKMatchState NewState);

	/** Enters the given round state if not set already */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EnterRoundState(ECSKRoundState NewState);

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

	/** Get the current state of the round */
	FORCEINLINE ECSKRoundState GetRoundState() const { return IsMatchInProgress() ? RoundState : ECSKRoundState::Invalid; }

	/** Get if we should start the match */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool ShouldStartMatch() const;

	/** Get if we should finish the match */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool ShouldEndMatch() const;

	/** Get if the match is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsMatchInProgress() const;

	/** Get if the match has ended */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasMatchFinished() const;

	/** Get if an action phase is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsActionPhaseInProgress() const;

protected:

	/** Match state notifies */
	virtual void OnStartWaitingPreMatch();
	virtual void OnMatchStart();
	virtual void OnMatchFinished();
	virtual void OnFinishedWaitingPostMatch();
	virtual void OnMatchAbort();

	/** Round state notifies */
	virtual void OnCollectionPhaseStart();
	virtual void OnFirstActionPhaseFinished();
	virtual void OnSecondActionPhaseFinished();
	virtual void OnEndRoundPhaseFinished();

private:

	/** Determines the match state change event to call based on previous and new match state */
	void HandleMatchStateChange(ECSKMatchState OldState, ECSKMatchState NewState);

	/** Determines the round state change event to called based on previous and new round state */
	void HandleRoundStateChange(ECSKRoundState OldState, ECSKRoundState NewState);

protected:

	/** The current state of the session */
	UPROPERTY(BlueprintReadOnly, Transient)
	ECSKMatchState MatchState;

	/** The current state of the round */
	UPROPERTY(BlueprintReadOnly, Transient)
	ECSKRoundState RoundState;

public:

	/** Will attempt to move active players castle towards given tile. Doing this will
	lock the ability for any other action to be made until castle reaches it's destination */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestCastleMove(ATile* Goal);

private:

	/** Starts movement request for active player. The request still has the chance of failing */
	bool ConfirmCastleMove(const FBoardPath& BoardPath);

	/** Finishes movement request for active player. This is after the castle has reached its destination */
	void FinishCastleMove(ATile* DestinationTile);

	/** Notify from active players castle that it has reached a new tile */
	UFUNCTION()
	void OnActivePlayersPathSegmentComplete(ATile* SegmentTile);

	/** Notify from the active players castle that is has reached its destination */
	UFUNCTION()
	void OnActivePlayersPathFollowComplete(ATile* DestinationTile);

	/** Helper function for checking if active player has moved onto the enemies portal tile.
	Will return true if the win condition has been met, false otherwise */
	bool PostCastleMoveCheckWinCondition(ATile* SegmentTile);

public:

	/** If we are waiting on a move request to finsih */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsPendingCastleMove() const { return bWaitingOnActivePlayerMoveAction; }

protected:

	/** The controller for the player performing their action phase */
	UPROPERTY(Transient)
	ACSKPlayerController* ActionPhaseActiveController;

private:

	/** If we are waiting for active players move action to complete */
	uint32 bWaitingOnActivePlayerMoveAction : 1;

	/** Delegate handle for when active players castle completes a segment of its path following */
	FDelegateHandle Handle_ActivePlayerPathSegment;

	/** Delegate handle for when active players castle reaches its destination tile */
	FDelegateHandle Handle_ActivePlayerPathComplete;




public:

	/** Called by game state when performing the coin flip. This will decide
	which players goes first-*/
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxGold;

	/** The amount of mana each player starts with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 StartingMana;

	/** How much mana is given to each player during the collection phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Resources, meta = (ClampMin = 0))
	int32 CollectionPhaseMana;

	/** The max amount of mana a player can hold at a time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxMana;

	/** The max amount of NORMAL towers a player can have constructed at once (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumTowers;

	/** The max amount of duplicates of the same NORMAL tower a player can construct (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumDuplicatedTowers;

	/** The max amount of LEGENDARY towers a player can have constructed at once (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumLegendaryTowers;

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

	/** Additional time to give players after having finished an action during their action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	float BonusActionPhaseTime;

	/** The min amount of tiles a player can move per action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 1))
	int32 MinTileMovements;

	/** The max amount of tiles a player can move per action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 1))
	int32 MaxTileMovements;

	/** The limit to how many buildings can be constructed per action phase (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxTowerConstructs;

protected:

	/** Notify that a client has disconnected */
	void OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver);

};
