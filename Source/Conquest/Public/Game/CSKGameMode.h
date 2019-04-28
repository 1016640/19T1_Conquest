// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "BoardPieceInterface.h"
#include "BoardTypes.h"
#include "CSKGameMode.generated.h"

class ACastle;
class ACastleAIController;
class ACSKPlayerController;
class ACSKPlayerState;
class APlayerStart;
class ASpellActor;
class ATile;
class ATower;
class IBoardPieceInterface;
class UHealthComponent;
class USpell;
class USpellCard;
class UTowerConstructionData;

using FCSKPlayerControllerArray = TArray<ACSKPlayerController*, TFixedAllocator<CSK_MAX_NUM_PLAYERS>>;

/** Delegate for when a sub spell has finished execution */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSubSpellFinished, ASpellActor*, FinishedSpell);

/** Contains information about a pending spell request */
USTRUCT()
struct CONQUEST_API FPendingSpellRequest
{
	GENERATED_BODY()

public:

	FPendingSpellRequest()
		: SpellIndex(0)
		, TargetTile(nullptr)
		, CalculatedCost(0)
		, AdditionalMana(0)
		, bIsSet(false)
	{

	}

public:

	/** Sets this request */
	void Set(TSubclassOf<USpellCard> InSpellCard, int32 InSpellIndex, ATile* InTargetTile, int32 InCalculatedCost, int32 InAdditionalMana)
	{
		SpellCard = InSpellCard;
		SpellIndex = InSpellIndex;
		TargetTile = InTargetTile;
		CalculatedCost = InCalculatedCost;
		AdditionalMana = InAdditionalMana;

		bIsSet = true;
	}

	/** Resets this request */
	void Reset()
	{
		SpellCard = nullptr;
		SpellIndex = 0;
		TargetTile = nullptr;
		CalculatedCost = 0;
		AdditionalMana = 0;

		bIsSet = false;
	}

	/** If this request is valid */
	FORCEINLINE bool IsValid() const { return bIsSet; }

public:

	/** The spell card to use */
	UPROPERTY()
	TSubclassOf<USpellCard> SpellCard;

	/** The index of the spell to use */
	int32 SpellIndex;

	/** The target of this spell */
	UPROPERTY()
	ATile* TargetTile;

	/** The calculated final cost of this request (discounted + additional mana) */
	int32 CalculatedCost;

	/** The additional mana that was provided to this spell */
	int32 AdditionalMana;

private:

	/** If this request is valid */
	uint8 bIsSet : 1;
};

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

	// Begin AGameModeBase Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool HasMatchStarted() const override;	
	// End AGameModeBase Interface

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

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

	/** Spawns a castle at portal corresponding with player*/
	ACastle* SpawnCastleAtPortal(ACSKPlayerController* Controller, TSubclassOf<ACastle> Class) const;

	/** Spawns the AI controller for given castle. Will auto possess the castle if successful */
	ACastleAIController* SpawnCastleControllerFor(ACastle* Castle) const;

	/** Finds a player start with matching tag */
	APlayerStart* FindPlayerStartWithTag(const FName& InTag) const;

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

	/** Ends the match */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EndMatch(ACSKPlayerController* WinningPlayer, ECSKMatchWinCondition MetCondition);

	/** Forcefully ends the match with no winner decided */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void AbortMatch();

public:

	/** Get the current state of the match */
	FORCEINLINE ECSKMatchState GetMatchState() const { return MatchState; }

	/** Get the current state of the round */
	FORCEINLINE ECSKRoundState GetRoundState() const { return IsMatchInProgress() ? RoundState : ECSKRoundState::Invalid; }

	/** Get the details about the winner */
	FORCEINLINE bool GetWinnerDetails(ACSKPlayerController*& OutWinner, ECSKMatchWinCondition& OutCondition) const
	{
		if (HasMatchFinished())
		{
			OutWinner = MatchWinner;
			OutCondition = MatchWinCondition;

			return true;
		}

		return false;
	}

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

	/** Get if the collection phase is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsCollectionPhaseInProgress() const;

	/** Get if an action phase is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsActionPhaseInProgress() const;

	/** Get if the end round phase is in progress */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsEndRoundPhaseInProgress() const;

protected:

	/** Match state notifies */
	virtual void OnStartWaitingPreMatch();
	virtual void OnCoinFlipStart();
	virtual void OnMatchStart();
	virtual void OnMatchFinished();
	virtual void OnFinishedWaitingPostMatch();
	virtual void OnMatchAbort();

	/** Round state notifies */
	virtual void OnCollectionPhaseStart();
	virtual void OnFirstActionPhaseStart();
	virtual void OnSecondActionPhaseStart();
	virtual void OnEndRoundPhaseStart();

private:

	/** Helper function for setting entering given round state after given delay */
	void EnterRoundStateAfterDelay(ECSKRoundState NewState, float Delay);

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

	/** The winner of the game when match has finished */
	UPROPERTY()
	ACSKPlayerController* MatchWinner;

	/** The condition the winner met in order to win */
	UPROPERTY()
	ECSKMatchWinCondition MatchWinCondition;

private:

	/** Timer handle to the small delay before entering a new round state */
	FTimerHandle Handle_EnterRoundState;




public:

	/** Function called when deciding which player gets to go first.
	This is called by the coin being flipped to determine what side to land on */
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
	bool GenerateCoinFlipWinner() const;

	/** Notify from the coin sequence actor that the sequence has finished */
	void OnStartingPlayerDecided(int32 WinningPlayerID);

public:

	/** Get the starting players ID */
	FORCEINLINE int32 GetStartingPlayersID() const { return StartingPlayerID; }

	/** Helper function for getting the player whose action phase it is based on starting player ID */
	ACSKPlayerController* GetActivePlayerForActionPhase(int32 Phase) const;

private:

	/** Helper function for initializing an action phase for given player */
	void UpdateActivePlayerForActionPhase(int32 Phase);

protected:

	/** The ID of the player who goes first */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	int32 StartingPlayerID;




private:

	/** Waits for an initial delay before starting collection phase sequence */
	void DelayThenStartCollectionPhaseSequence();

	/** Starts the collection phase resource sequence */
	void StartCollectionPhaseSequence();

	/** Timer callback that gets called when collection phase has been deemed to last to long. This is a
	safety measure to avoid the potential case of NotifyCollectionPhaseSequenceFinished() not being called */
	void ForceCollectionPhaseSequenceEnd();

private:

	/** Resets both players resources based on default resource settings */
	void ResetResourcesForPlayers();

	/** Updates both players resources based on default
	resource collection and tower resource bonuses */
	void CollectResourcesForPlayers();

	/** Resets the resources player has to defaults.
	Player ID is passed for logging purposes only, it should not be used to determine which player is being updated */
	void ResetPlayerResources(ACSKPlayerController* Controller, int32 PlayerID);

	/** Gives player default resources, as well as any additional resources provided by towers.
	Player ID is passed for logging purposes only, it should not be used to determine which player is being updated */
	void UpdatePlayerResources(ACSKPlayerController* Controller, int32 PlayerID);

public:

	/** Notify that collection phase tallies have completed client side */
	void NotifyCollectionPhaseSequenceFinished(ACSKPlayerController* Player);

private:

	/** Bitset for tracking which players have finished collection phase tally event */
	uint32 CollectionSequenceFinishedFlags : 2;

	/** Timer handle for managing the collection phase sequences. This is both as an initial delay
	before updating resources and a limit timer in-case clients take to long to finish their sequence */
	FTimerHandle Handle_CollectionSequences;




public:

	/** Will attempt to end active players action phase if active player has fulfilled action requirements */
	bool RequestEndActionPhase(bool bTimeOut = false);

	/** Will attempt to move active players castle towards given tile. Doing this will
	lock the ability for any other action to be made until castle reaches it's destination */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestCastleMove(ATile* Goal);

	/** Will attempt to build the given type of tower for active player at given tile */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestBuildTower(TSubclassOf<UTowerConstructionData> TowerData, ATile* Tile);

	/** Will attempt to cast the given type of spell for active player. This function should
	not be used for quick effects, but only for players using spells during their action phase.
	This function will return true even if we start waiting for the opposing player to select a counter spell (Quick Effect) */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestCastSpell(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* TargetTile, int32 AdditionalMana = 0);

	/** Will attempt to cast the given spell as a counter to a pending spell cast. This function should
	only be used for quick effects, for normal action phase spells, use RequestCastSpell */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestCastQuickEffect(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* TargetTile, int32 AdditionalMana = 0);

	/** Will attempt to end the quick effect spell selection without casting a counter */
	bool RequestSkipQuickEffect();

	/** Will attempt to cast the current pending bonus spell */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool RequestCastBonusSpell(ATile* TargetTile);

	/** Will attempt to skip the bonus spell target selecting without casting the spell */
	bool RequestSkipBonusSpell();

public:
	
	/** DO NOT CALL THIS. Casts a sub spell for current activated spell. 
	Cost can be changed by setting a value equal or greater than zero */
	ASpellActor* CastSubSpellForActiveSpell(TSubclassOf<USpell> SubSpell, ATile* TargetTile, int32 AdditionalMana = 0, int32 OverrideCost = -1);

	/** DO NOT CALL THIS. Notify that executing spell has finished */
	void NotifyCastSpellFinished(ASpellActor* FinishedSpell, bool bWasCancelled);

private:

	/** Disables the action mode for active player. Will end this phase if no actions remain */
	void DisableActionModeForActivePlayer(ECSKActionPhaseMode ActionMode);

	/** ----- MOVE ACTION ----- */

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

	/** ----- BUILD ACTION ----- */

	/** Finalizes build request for active player. The request still has a chance of failing */
	bool ConfirmBuildTower(ATower* Tower, ATile* Tile, UTowerConstructionData* ConstructData);

	/** Finishes the build request for active player. This is after the tower has finished its emerge sequence */
	void FinishBuildTower();

	/** Spawns a new tower of type for given player at tile */
	ATower* SpawnTowerFor(TSubclassOf<ATower> Template, ATile* Tile, UTowerConstructionData* ConstructData, ACSKPlayerState* PlayerState) const;

	/** Notify from timer that we should start tower build sequence */
	void OnStartActivePlayersBuildSequence();

	/** Notify from the active players pending tower that is has completed its build seuqence */
	UFUNCTION()
	void OnPendingTowerBuildSequenceComplete();

	/** ----- SPELL ACTION ----- */

	/** Starts casting the spell. Will determine the instigator based on passed context */
	bool ConfirmCastSpell(USpell* Spell, USpellCard* SpellCard, ASpellActor* SpellActor, int32 FinalCost, 
		ATile* Tile, EActiveSpellContext Context, bool bConsumeOnlyQuickEffect = false);

	/** Finishes the spell currently be cast */
	void FinishCastSpell(bool bIgnoreQuickEffectCheck = false, bool bIgnoreBonusCheck = false);

	/** Spawns the spell actor for given spell at tile */
	ASpellActor* SpawnSpellActor(USpell* Spell, ATile* Tile, int32 FinalCost, int32 AdditionalMana, ACSKPlayerState* PlayerState) const;

	/** Notify from timer that we should execute the spell cast */
	void OnStartActiveSpellCast();

	/** Notify from timer that we should begin execution of given sub spell */
	void OnStartSubSpellCast(ASpellActor* SpellActor);

	/** Saves an incoming action spell request to be cast at a later time.
	Will notify the opposing player to select a nullify spell request*/
	void SaveActionSpellRequestAndWaitForCounterSelection(TSubclassOf<USpellCard> InSpellCard, 
		int32 InSpellIndex, ATile* InTargetTile, int32 InFinalCost, int32 AdditionalMana);

	/** Post action spell check to determine if the opposing player can
	select a post quick effect spell. Get if a quick effect is being cast */
	bool PostCastSpellWaitForCounterSelection();

	/** Post action spell check to determine if a bonus spell should be cast. Get if a bonus spell is being cast */
	bool PostCastSpellActivateBonusSpell();

	/** Saves the incoming bonus spell and informs casting player to choose a target */
	void SaveBonusSpellAndWaitForTargetSelection(TSubclassOf<USpell> InBonusSpell);

	/** Notifies players and game state to wait for quick effect selection */
	void OnStartWaitForCounterSelection(bool bNullify, TSubclassOf<USpell> SpellToCounter, ATile* TargetTile) const;

public:

	/** Get the controller for player whose action phase it is */
	FORCEINLINE ACSKPlayerController* GetActionPhaseActiveController() const { return ActionPhaseActiveController; }

	/** If we are waiting on an action request to finish */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsWaitingForAction() const { return IsWaitingForCastleMove() || IsWaitingForBuildTower() || IsWaitingForSpellCast(); }

	/** If we are waiting on a move request to finsih */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsWaitingForCastleMove() const { return bWaitingOnActivePlayerMoveAction; }

	/** If we are waiting on a build request to finish */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsWaitingForBuildTower() const { return bWaitingOnActivePlayerBuildAction; }

	/** If we are waiting on a spell action to finish */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool IsWaitingForSpellCast() const { return bWaitingOnSpellAction; }

private:

	/** If we should allow any action requests */
	FORCEINLINE bool ShouldAcceptRequests() const
	{
		return !(IsWaitingForAction() || 
			!(bWaitingOnNullifyQuickEffectSelection || bWaitingOnPostQuickEffectSelection || bWaitingOnBonusSpellSelection));
	}

	/** Resets all wait action flags */
	FORCEINLINE void ResetWaitingOnActionFlags()
	{
		bWaitingOnActivePlayerMoveAction = false;
		bWaitingOnActivePlayerBuildAction = false;
		bWaitingOnSpellAction = false;

		ActivePlayerPendingTower = nullptr;
		ActivePlayerPendingTowerTile = nullptr;
		
		bWaitingOnNullifyQuickEffectSelection = false;
		ActiveSpell = nullptr;
		ActiveSpellCard = nullptr;
		ActiveSpellActor = nullptr;
		ActiveSpellTarget = nullptr;
		ActiveSpellContext = EActiveSpellContext::None;
		ActiveSubSpellActors.Empty();
		bWaitingOnBonusSpellSelection = false;
		BonusSpellContext = EActiveSpellContext::None;

		ActivePlayerPendingSpellRequest.Reset();
	}

public:

	/** Event for when a sub spell has finished. This does
	not get called for sub spells that were cancelled early */
	FSubSpellFinished OnSubSpellFinished;

protected:

	/** The controller for the player performing their action phase */
	UPROPERTY(Transient)
	ACSKPlayerController* ActionPhaseActiveController;

private:

	/** If we are waiting for active players move action to complete */
	uint32 bWaitingOnActivePlayerMoveAction : 1;

	/** If we are waiting for active players build action to complete */
	uint32 bWaitingOnActivePlayerBuildAction : 1;

	/** If we are waiting for actives players spell action to complete */
	uint32 bWaitingOnSpellAction : 1;

	/** ----- MOVE ACTION ----- */

	/** Delegate handle for when active players castle completes a segment of its path following */
	FDelegateHandle Handle_ActivePlayerPathSegment;

	/** Delegate handle for when active players castle reaches its destination tile */
	FDelegateHandle Handle_ActivePlayerPathComplete;

	/** ----- BUILD ACTION ----- */

	/** The tower that is in the process of being built. Keeping this here so we can
	wait for it to initially replicate to all clients before placing on the board */
	UPROPERTY()
	ATower* ActivePlayerPendingTower;

	/** The tile the pending tower needs to be palced on.  Keeping this here so we can
	wait for it the tower to initially replicate to all clients before placing on the board */
	UPROPERTY()
	ATile* ActivePlayerPendingTowerTile;

	/** Timer handle for when activating the build sequence for pending tower */
	FTimerHandle Handle_ActivePlayerStartBuildSequence;

	/** Delegate handle for when pending towers build sequence has completed */
	FDelegateHandle Handle_ActivePlayerBuildSequenceComplete;

	/** ----- SPELL ACTION ----- */

	/** If we are waiting on a nullify quick effect selection  */
	uint32 bWaitingOnNullifyQuickEffectSelection : 1;

	/** If we are waiting on a post action quick effect selection */
	uint32 bWaitingOnPostQuickEffectSelection : 1;

	/** If we are waiting on the player who casted the current spell to select a target for the bonus spell */
	uint32 bWaitingOnBonusSpellSelection : 1;

	/** The active players pending spell request. This saves an incoming action spell
	request while the opposing player selects a potential quick effect spell */
	UPROPERTY(Transient)
	FPendingSpellRequest ActivePlayerPendingSpellRequest;

	/** The spell being cast by active player (Points to default object). */
	UPROPERTY()
	USpell* ActiveSpell;

	/** The spell card associated with the spell being cast (Points to default object). */
	UPROPERTY()
	USpellCard* ActiveSpellCard;

	/** Instanced actor provided executing the spells logic */
	UPROPERTY()
	ASpellActor* ActiveSpellActor;

	/** The target for the active spell */
	UPROPERTY()
	ATile* ActiveSpellTarget;

	/** The context of the spell being cast */
	EActiveSpellContext ActiveSpellContext;

	/** Any secondary spell actors that have been spawned by the current spell actor */
	UPROPERTY()
	TArray<ASpellActor*> ActiveSubSpellActors; // TODO: Could be a TSet

	/** The bonus spell that is waiting to be cast */
	UPROPERTY()
	TSubclassOf<USpell> PendingBonusSpell;

	/** The context of the spell that granted the bonus spells execution */
	EActiveSpellContext BonusSpellContext;

	/** Timer handle for when waiting for a spell to replicate before executing its effects */
	FTimerHandle Handle_ExecuteSpellCast;

public:

	/** Notify that tower running its end round phase event has finished */
	void NotifyEndRoundActionFinished(ATower* Tower);

private:

	/** Checks all towers determine order to execute end round actions. Get if any actions are ready to be performed */
	bool PrepareEndRoundActionTowers();

	/** Attempts to start the action for end round tower at given index. Get if starting the next towers action was successfull */
	bool StartRunningTowersEndRoundAction(int32 Index);

	/** Sets delay of given time before attempting to start next tower action */
	void StartNextEndRoundActionAfterDelay(float Delay);

	/** Callback from delay end round action */
	void OnStartNextEndRoundAction();

private:

	/** The list of towers that will run the end round action during this end
	round phase. This array is sorted by order of action priority (see Tower.h).*/
	UPROPERTY()
	TArray<ATower*> EndRoundActionTowers;

	/** The index of the current tower running the end round action */
	int32 EndRoundRunningTower;

	/** If we are current initiating a towers action. This helps 
	dealing with towers whose actions conclude immediately */
	uint32 bInitiatingTowerEndRoundAction : 1;

	/** If a tower is running its end round action event */
	uint32 bRunningTowerEndRoundAction : 1;

	/** Timer handle for creating small delays between end round actions */
	FTimerHandle Handle_DelayEndRoundAction;

public:

	/** Clamps value based on max gold allowed */
	UFUNCTION(BlueprintPure, Category = Resources)
	int32 ClampGoldToLimit(int32 Gold) const { return FMath::Clamp(Gold, 0, MaxGold); }
	
	/** Clamps value based on max mana allowed */
	UFUNCTION(BlueprintPure, Category = Resources)
	int32 ClampManaToLimit(int32 Mana) const { return FMath::Clamp(Mana, 0, MaxMana); }

	/** Gives gold to given player state clamped to gold limit. Amount can be negative */
	UFUNCTION(BlueprintCallable, Category = Resources)
	void GiveGoldToPlayer(ACSKPlayerState* PlayerState, int32 Amount) const;

	/** Gives mana to given player state clamped to mana limit. Amount can be negative */
	UFUNCTION(BlueprintCallable, Category = Resources)
	void GiveManaToPlayer(ACSKPlayerState* PlayerState, int32 Amount) const;

	/** Get the max number of NORMAL towers the player is allowed to build */
	FORCEINLINE int32 GetMaxNumTowers() const { return MaxNumTowers; }

	/** Get the max number of duplicate NORMAL towers the player is allowed to build */
	FORCEINLINE int32 GetMaxNumDuplicatedTowers() const { return MaxNumDuplicatedTowers; }

	/** Get the max number of duplicate NORMAL tower types the player is allowed to build */
	FORCEINLINE int32 GetMaxNumDuplicatedTowerTypes() const { return MaxNumDuplicatedTowerTypes; }

	/** Get the max number of LEGENDARY towers the player is allowed to build */
	FORCEINLINE int32 GetMaxNumLegendaryTowers() const { return MaxNumLegendaryTowers; }

	/** Get the max range the player can build a tower away from their castle */
	FORCEINLINE int32 GetMaxBuildRange() const { return MaxBuildRange; }

	/** Get the towers available for use */
	FORCEINLINE const TArray<TSubclassOf<UTowerConstructionData>>& GetAvailableTowers() const { return AvailableTowers; }

protected:

	#if WITH_EDITORONLY_DATA
	/** Debug color to give to player 1 in a PIE session */
	UPROPERTY(EditAnywhere, Category = "Rules|Debug")
	FColor P1AssignedColor;

	/** Debug color to given to player 2 in a PIE session */
	UPROPERTY(EditAnywhere, Category = "Rules|Debug")
	FColor P2AssignedColor;
	#endif WITH_EDITORONLY_DATA

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

	/** The max amount of duplicates of a single NORMAL tower a player can construct (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumDuplicatedTowers;

	/** The max amount of duplicated types of all NORMAL towers player can have (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumDuplicatedTowerTypes;

	/** The max amount of LEGENDARY towers a player can have constructed at once (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	int32 MaxNumLegendaryTowers;

	/** The max range players can be a tower away from their castle at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 1))
	int32 MaxBuildRange;

	/** The types of towers that can be built */
	// TODO: replace this with primary asset IDs for towers, when game state gets it
	// it will load them in manually via the asset manager
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	TArray<TSubclassOf<UTowerConstructionData>> AvailableTowers;

	/** The max amount of spells a player can use per turn with by default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	int32 MaxSpellUses;

	/** The max amount of spells cards a player can have in their hand */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	int32 MaxSpellCardsInHand;

	/** The spell cards that can be cast */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	TArray<TSubclassOf<USpellCard>> AvailableSpellCards;

	/** The spells that auto activate if a player casts a spell
	with elements that match the tile their castle is on */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	TMap<ECSKElementType, TSubclassOf<USpell>> BonusElementalSpells;

public:

	/** Get the time an action phase lasts */
	UFUNCTION(BlueprintPure, Category = Rules)
	float GetActionPhaseTime() const { return ActionPhaseTime; }

	/** Get the bonus time to add to action phase time after finishing an action */
	UFUNCTION(BlueprintPure, Category = Rules)
	float GetBonusActionPhaseTime() const { return BonusActionPhaseTime; }

	/** Get the min amount of tiles that must be traversed per action phase */
	FORCEINLINE int32 GetMinTileMovementsPerTurn() const { return MinTileMovements; }

	/** Get the max amount of tiles that can be traversed per action phase */
	FORCEINLINE int32 GetMaxTileMovementsPerTurn() const { return MaxTileMovements; }

	/** Get the time a quick effect selection lasts */
	UFUNCTION(BlueprintPure, Category = Rules)
	float GetQuickEffectCounterTime() const { return QuickEffectCounterTime; }

	/** Get the time a bonus spell selection lasts */
	UFUNCTION(BlueprintPure, Category = Rules)
	float GetBonusSpellSelectTime() const { return BonusSpellSelectTime; }

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

	/** If players are only allowed to request one move action per turn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules)
	uint32 bLimitOneMoveActionPerTurn : 1;

	/** Random stream used for shuffling a players spell deck */
	FRandomStream DeckReshuffleStream;

	/** The time the player has to select a quick effect spell when other player is casting a spell (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	float QuickEffectCounterTime;

	/** The time the player has to select a target when granted a bonus spell that requires one (zero means indefinite) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rules, meta = (ClampMin = 0))
	float BonusSpellSelectTime;

	/** How long we wait before starting the match (starting the coin flip).
	A delay of two seconds or greater is recommended to allow actors to replicate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = Rules, meta = (ClampMin=1))
	float InitialMatchDelay;

	/** How long the post match phase should wait for before returning to lobby */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = Rules, meta = (ClampMin = 1))
	float PostMatchDelay;

protected:

	/** Callback for when a tower/castle has taken damage. This
	function checks if given tower/castle has been destroyed */
	UFUNCTION()
	void OnBoardPieceHealthChanged(UHealthComponent* HealthComp, int32 NewHealth, int32 Delta);

	/** Clears both health report arrays */
	void ClearHealthReports();

	/** Caches all the active action health reports then clearing it for a new action */
	void CacheAndClearHealthReports();

protected:

	/** Reports of health changed during the current action, sorted in the order
	they were recieved. Actions are a spell cast or a towers end round phase action */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "CSK|Game")
	TArray<FHealthChangeReport> ActiveActionHealthReports;

	/** The health change reports from the previous action */
	// TODO: The game state needs these, so clients can also access them!
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "CSK|Game")
	TArray<FHealthChangeReport> PreviousActionHealthReports;

protected:

	/** Notify that a client has disconnected */
	void OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver);
};
