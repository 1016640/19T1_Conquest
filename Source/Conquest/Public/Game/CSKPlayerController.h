// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerController.h"
#include "BoardTypes.h"
#include "CSKPlayerController.generated.h"

class ACastle;
class ACastleAIController;
class ACoinSequenceActor;
class ACSKHUD;
class ACSKPawn;
class ACSKPlayerCameraManager;
class ACSKPlayerState;
class ATile;
class ATower;
class USpell;
class USpellCard;
class UTowerConstructionData;

/** Delegate for checking if player can select the tile */
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FCanSelectTileSignature, ATile*, HoveredTile);

/** Delegate for when the a tile has been selected */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSelectTileSignature, ATile*, SelectedTile);

/** Struct containing tallied amount of resources to give to a player */
USTRUCT(BlueprintType)
struct CONQUEST_API FCollectionPhaseResourcesTally
{
	GENERATED_BODY()

public:

	FCollectionPhaseResourcesTally()
	{
		Reset();
	}

	FCollectionPhaseResourcesTally(int32 InGold, int32 InMana, bool bInDeckReshuffled, TSubclassOf<USpellCard> InSpellCard)
	{
		Gold = InGold;
		Mana = InMana;
		bDeckReshuffled = bInDeckReshuffled;
		SpellCard = InSpellCard;
	}

public:

	/** Resets tally */
	FORCEINLINE void Reset()
	{
		Gold = 0;
		Mana = 0;
		bDeckReshuffled = false;
		SpellCard = nullptr;
	}

public:

	/** Gold tallied */
	UPROPERTY(BlueprintReadOnly)
	int32 Gold; // (uint8?)

	/** Mana tallied */
	UPROPERTY(BlueprintReadOnly)
	int32 Mana; // (uint8?)

	/** If spell deck was reshuffled */
	UPROPERTY(BlueprintReadOnly)
	uint8 bDeckReshuffled : 1;

	/** The spell card the player picked up.
	Will be invalid if no card was picked up */
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<USpellCard> SpellCard;
};

/**
 * Controller for handling communication events between players and the server
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPlayerController : public APlayerController
{
	GENERATED_BODY()

	friend class ACSKPlayerCameraManager;
	
public:

	ACSKPlayerController();

public:

	// Begin APlayerController Interface
	virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass) override;
	// End APlayerController Interface

	// Begin AController Interface
	virtual void OnRep_Pawn() override;
	// End AController Interface

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	// End APlayerController Interface

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** This controllers player ID, this should never be altered */
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, Replicated)
	int32 CSKPlayerID;

public:

	/** Set the tower this player wants to build */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SetSelectedTower(TSubclassOf<UTowerConstructionData> InConstructData);

	/** Set the spell this player wants to cast */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SetSelectedSpellCard(TSubclassOf<USpellCard> InSpellCard, int32 InSpellIndex);

	/** Set the additional mana the player wants to spend for spells */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SetSelectedAdditionalMana(int32 InAdditionalMana);

public:

	/** Get if player has given tower construction data selected */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasTowerSelected(TSubclassOf<UTowerConstructionData> InConstructData) const;

	/** Get if player has given spell selected */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool HasSpellSelected(TSubclassOf<USpell> InSpell) const;

public:

	/** Get possessed pawn as a CSK pawn */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPawn* GetCSKPawn() const;

	/** Get player state as CSK player state */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerState* GetCSKPlayerState() const;

	/** Get camera manager as CSK camera manager */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerCameraManager* GetCSKPlayerCameraManager() const;

	/** Get cached CSK HUD (only valid on clients) */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKHUD* GetCSKHUD() const;

	/** Get the tile currently under this controllers mouse. This only works only local player controllers */
	UFUNCTION(BlueprintCallable, Category = CSK)
	ATile* GetTileUnderMouse() const;

protected:

	/** Event for when the player has hovered over a new tile. This
	can be null, to signal that the player is no longer over a tile */
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
	void OnNewTileHovered(ATile* NewTile);

private:

	/** Notify from camera manager that fade out/in has finished */
	void NotifyFadeOutInSequenceFinished();

public:

	/** Sets whether we can select tiles */
	void SetCanSelectTile(bool bEnable);

private:

	/** Attempts to perform an action using the current hovered tile */
	void SelectTile();

	/** Resets our camera to focus on our castle */
	void ResetCamera();

protected:

	/** Our HUD class as a CSK HUD */
	UPROPERTY()
	ACSKHUD* CachedCSKHUD;

private:

	/** Executes the on select tile event on the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ExecuteCustomOnSelectTile(ATile* SelectedTile);

public:

	/** Event to check if this player can select given tile. 
	This is executed on both the client and the server */
	FCanSelectTileSignature CustomCanSelectTile;

	/** Event for when this player selects the given tile and can select
	tile returned true (if bound). This is executed only on the server */
	FOnSelectTileSignature CustomOnSelectTile;

protected:

	/** The tile we are currently hovering over (only valid on the client) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* HoveredTile;

	/** If we are accepting input via select tile (only valid on the client) */
	UPROPERTY(Transient)
	uint32 bCanSelectTile : 1;

	/** The tower the player has selected to build */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = CSK)
	TSubclassOf<UTowerConstructionData> SelectedTowerConstructionData;

	/** The spell the player has selected to cast */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = CSK)
	TSubclassOf<USpellCard> SelectedSpellCard;

	/** The index of the spell to use of the selected spell card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = CSK)
	int32 SelectedSpellIndex;

	/** The additional mana this player is willing to spend when casting spells */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = CSK)
	int32 SelectedSpellAdditionalMana;

public:

	/** Set the castle this player manages. This only works on the server */
	void SetCastleController(ACastleAIController* InController);

public:

	/** Get the castle controller managing our castle */
	FORCEINLINE ACastleAIController* GetCastleController() const { return CastleController; }

	/** Get the castle we own */
	FORCEINLINE ACastle* GetCastlePawn() const { return CastlePawn; }

protected:
	
	/** The castle AI controller managing this players castle. This is only valid on the server */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	ACastleAIController* CastleController;

	/** The castle pawn itself */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = CSK)
	ACastle* CastlePawn;

protected:

	/** Notify that the round state has changed */
	UFUNCTION()
	void OnRoundStateChanged(ECSKRoundState NewState);

public:

	/** Notify to transition to the coin sequence */
	UFUNCTION(Client, Reliable)
	void Client_TransitionToCoinSequence(ACoinSequenceActor* SequenceActor);

	/** Notify that the winner of the coin toss has been decided */
	UFUNCTION(Client, Reliable)
	void Client_OnStartingPlayerDecided(bool bStartingPlayer);

	/** Notify to transition to the board (our pawn) */
	UFUNCTION(Client, Reliable)
	void Client_TransitionToBoard();

	/** Notify that the match has started */
	UFUNCTION(Client, Reliable)
	void Client_OnMatchStarted();

	/** Notify that the match has concluded */
	UFUNCTION(Client, Reliable)
	void Client_OnMatchFinished(bool bIsWinner);

private:

	/** Informs the server that we have either finished
	transitioning to the coin sequence or back to the board */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TransitionSequenceFinished();

public:

	/** Notify that we have collected resources during collection phase */
	UFUNCTION(Client, Reliable)
	void Client_OnCollectionPhaseResourcesTallied(FCollectionPhaseResourcesTally TalliedResources);

protected:

	/** Event for when this players collection phase resources has been tallied. This runs on the client and should
	ultimately call FinishCollectionTallyEvent when any local events have concluded (e.g. displaying the tallies) */
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
	void OnCollectionResourcesTallied(const FCollectionPhaseResourcesTally& TalliedResources);

	/** Finishes the collection phase event. This must be called after collection resources tallied event */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void FinishCollectionSequenceEvent();

private:

	/** Notifies server that local client has finished collecting resources */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FinishCollecionSequence();

private:

	/** If we are waiting on the collection phase tally to conclude */
	uint32 bWaitingOnTallyEvent : 1;

public:

	/** Enables/Disables this players action phase */
	void SetActionPhaseEnabled(bool bEnabled);

	/** Ends our action phase if we are allowed to */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void EndActionPhase();

	/** Sets the action mode for this player */
	void SetActionMode(ECSKActionPhaseMode NewMode, bool bClientOnly = false);

	/** Enters the given action mode */
	UFUNCTION(BlueprintCallable, Category = CSK, meta = (DisplayName="Set Action Mode"))
	void BP_SetActionMode(ECSKActionPhaseMode NewMode);

	/** Enables/Disables this players nullify quick effect selection */
	void SetNullifyQuickEffectSelectionEnabled(bool bEnable);

	/** Enables/Disables this players post quick effect selection */
	void SetPostQuickEffectSelectionEnabled(bool bEnable);

	/** Resets both quick effect selection options */
	void ResetQuickEffectSelections();

	/** Enables/Disables this players bonus spell selection */
	void SetBonusSpellSelectionEnabled(bool bEnable);

private:

	/** Set action mode on the server */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetActionMode(ECSKActionPhaseMode NewMode);

public:

	/** If this player is currently performing their action phase */
	FORCEINLINE bool IsPerformingActionPhase() const { return bIsActionPhase; }

	/** If this player is allowed to end their action phase */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool CanEndActionPhase() const;

	/** Get if this player cen enter given action mode */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool CanEnterActionMode(ECSKActionPhaseMode ActionMode) const;

	/** If this player is allowed to request a castle move */
	bool CanRequestCastleMoveAction() const;

	/** If this player is allowed to request a tower construction */
	bool CanRequestBuildTowerAction() const;

	/** If this player is allowed to request a spell cast */
	bool CanRequestCastSpellAction() const;

protected:

	/** Event for when the action phase mode has changed */
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
	void OnSelectionModeChanged(ECSKActionPhaseMode NewMode);

private:

	/** Notify the is action phase has been replicated */
	UFUNCTION()
	void OnRep_bIsActionPhase();

	/** Notify that remaining actions has been replicated */
	UFUNCTION()
	void OnRep_RemainingActions();

	/** Notify that can select quick effect has been replicated */
	UFUNCTION()
	void OnRep_bCanSelectNullifyQuickEffect();

	/** Notify that can select quick effect target has been replicated */
	UFUNCTION()
	void OnRep_bCanSelectPostQuickEffect();

	/** Notify than can select bonus spell target has been replicated */
	UFUNCTION()
	void OnRep_bCanSelectBonusSpellTarget();

protected:

	/** If it is our action phase */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsActionPhase, Category = CSK)
	uint32 bIsActionPhase : 1;

	/** The current action mode we are in */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	ECSKActionPhaseMode SelectedAction;

	/** The actions we can still during our action phase */
	UPROPERTY(ReplicatedUsing = OnRep_RemainingActions)
	ECSKActionPhaseMode RemainingActions;

	/** If this player is allowed to select a nullify counter spell */
	UPROPERTY(ReplicatedUsing = OnRep_bCanSelectNullifyQuickEffect)
	uint32 bCanSelectNullifyQuickEffect : 1;

	/** If this player is allowed to select a post action spell counter spell */
	UPROPERTY(ReplicatedUsing = OnRep_bCanSelectPostQuickEffect)
	uint32 bCanSelectPostQuickEffect : 1;

	/** If this player can select a bonus spell target */
	UPROPERTY(ReplicatedUsing = OnRep_bCanSelectBonusSpellTarget)
	uint32 bCanSelectBonusSpellTarget : 1;

	/** If we should ignore the CanSelect flags for spells. This is used
	to allow input during spell actions (as spell actors can bind input) */
	uint32 bIgnoreCanSelectSpellFlags : 1;

public:

	/** Notify that an action phase move request has been confirmed */
	UFUNCTION(Client, Reliable)
	void Client_OnCastleMoveRequestConfirmed(ACastle* MovingCastle);

	/** Notify that an action phase move request has finished */
	UFUNCTION(Client, Reliable)
	void Client_OnCastleMoveRequestFinished();

	/** Notify that an action phase build request has been confirmed */
	UFUNCTION(Client, Reliable)
	void Client_OnTowerBuildRequestConfirmed(ATile* TargetTile);

	/** Notify that an action phase build request has finished */
	UFUNCTION(Client, Reliable)
	void Client_OnTowerBuildRequestFinished();

	/** Notify that a spell cast has been confirmed */
	UFUNCTION(Client, Reliable)
	void Client_OnCastSpellRequestConfirmed(EActiveSpellContext SpellContext, ATile* TargetTile);

	/** Notify that a spell cast has finished */
	UFUNCTION(Client, Reliable)
	void Client_OnCastSpellRequestFinished(EActiveSpellContext SpellContext);

	/** Notify that this player is able to counter an incoming spell cast
	(and if the spell is selection is a nullify or post action counter )*/
	UFUNCTION(Client, Reliable)
	void Client_OnSelectCounterSpell(bool bNullify, TSubclassOf<USpell> SpellToCounter, ATile* TargetTile);

	/** Notify that this players spell request is pending as the opposing player is selecting a counter */
	UFUNCTION(Client, Reliable)
	void Client_OnWaitForCounterSpell(bool bNullify);

	/** Notify that this player is able to select a tile to use a bonus spell on */
	UFUNCTION(Client, Reliable)
	void Client_OnSelectBonusSpellTarget(TSubclassOf<USpell> BonusSpell);

	/** Notify that the opposing player is able to select a tile to use a bonus spell on */
	UFUNCTION(Client, Reliable)
	void Client_OnWaitForBonusSpell();

	/** Disable the ability to use the given action mode for the rest of this round.
	Get if no action remains (always returns false on client or if not in action phase) */
	bool DisableActionMode(ECSKActionPhaseMode ActionMode);

	/** Notify that the tower on given tile is starting is end round action */
	UFUNCTION(Client, Reliable)
	void Client_OnTowerActionStart(ATile* TileWithTower);

public:

	/** Moves our castle to the currently hovered tile.
	This will only work if running on local players controller */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void MoveCastleToHoveredTile();

	/** Builds the given tower at the currently hovered tile. 
	This will only work if running on local players controller */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void BuildTowerAtHoveredTile(TSubclassOf<UTowerConstructionData> TowerConstructData);

	/** Casts the given spell (of spell card) at the currently hovered tile. 
	This will only work if running on local players controller */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void CastSpellAtHoveredTile(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, int32 AdditionalMana = 0);

	/** Casts the given spell (of spell card) as a quick effect at the currently hovered tile */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void CastQuickEffectSpellAtHoveredTile(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, int32 AdditionalMana = 0);

	/** Skips the opportunity to cast a quick effect spell */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SkipQuickEffectSpell();

	/** Casts the pending bonus spell at the currently hovered tile */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void CastBonusElementalSpellAtHoveredTile();

	/** Skips the opportunity to cast a bonus elemental spell */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SkipBonusElementalSpell();

private:

	/** The bonus spell we are allowed to cast. This is only valid on the client */
	UPROPERTY()
	TSubclassOf<USpell> PendingBonusSpell;

protected:

	/** Makes a request to the server to end our action phase */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_EndActionPhase();

	/** Makes a request to move our castle towards the goal tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastleMoveAction(ATile* Goal);

	/** Makes a request to build a tower at given tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestBuildTowerAction(TSubclassOf<UTowerConstructionData> TowerConstructData, ATile* Target);

	/** Makes a request to cast a spell at given tile (with additional mana cost) */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastSpellAction(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana);

	/** Makes a request to cast a counter spell at given tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastQuickEffectAction(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana);

	/** Makes a request to skip selecting a counter spell */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SkipQuickEffectSelection();

	/** Makes a request to use bonus spell at selected tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastBonusSpellAction(ATile* Target);

	/** Makes a request to skip using a bonus elemental spell */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SkipBonusSpellSelection();

public:

	/** Get the list of towers this player can build */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetBuildableTowers(TArray<TSubclassOf<UTowerConstructionData>>& OutTowers) const;

	/** Get the list of spells this player can cast (in hand) */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetCastableSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const;

	/** Get the list of quick effect spells this player can cast (in hand) */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetCastableQuickEffectSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards, bool bNullifySpells = true) const;

	/** Get the bonus elemental spell this player can cast */
	UFUNCTION(BlueprintPure, Category = CSK)
	TSubclassOf<USpell> GetCastableBonusElementalSpell() const { return PendingBonusSpell; }

private:

	/** Sets all tiles in selected action candidates to given selection state */
	void SetTileCandidatesSelectionState(ETileSelectionState SelectionState) const;

private:

	/** The tiles that are selectable for current selected action. Cast spell is different,
	as it will only contain the hovered tile if we are allowed to cast the selected spell on it */
	UPROPERTY(Transient)
	TArray<ATile*> SelectedActionTileCandidates;
};
