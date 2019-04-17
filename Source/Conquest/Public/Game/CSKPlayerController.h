// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerController.h"
#include "BoardTypes.h"
#include "CSKPlayerController.generated.h"

class ACastle;
class ACastleAIController;
class ACSKHUD;
class ACSKPawn;
class ACSKPlayerState;
class ATile;
class ATower;
class USpell;
class USpellCard;
class UTowerConstructionData;

/** Struct containing tallied amount of resources to give to a player */
USTRUCT()
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
	UPROPERTY()
	int32 Gold; // (uint8?)

	/** Mana tallied */
	UPROPERTY()
	int32 Mana; // (uint8?)

	/** If spell deck was reshuffled */
	UPROPERTY()
	uint8 bDeckReshuffled : 1;

	/** The spell card the player picked up.
	Will be invalid if no card was picked up */
	UPROPERTY()
	TSubclassOf<USpellCard> SpellCard;
};

/**
 * Controller for handling communication events between players and the server
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	ACSKPlayerController();

public:

	// Begin APlayerController Interface
	virtual void ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass) override;
	// End APlayerController Interface

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

	/** Get possessed pawn as a CSK pawn */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPawn* GetCSKPawn() const;

	/** Get player state as CSK player state */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerState* GetCSKPlayerState() const;

	/** Get cached CSK HUD (only valid on clients) */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKHUD* GetCSKHUD() const;

	/** Get the tile currently under this controllers mouse. This only works only local player controllers */
	UFUNCTION(BlueprintCallable, Category = CSK)
	ATile* GetTileUnderMouse() const;

private:

	/** Attempts to perform an action using the current hovered tile */
	void SelectTile();

	/** Resets our camera to focus on our castle */
	void ResetCamera();

	/** Sets whethere we can select tiles */
	void SetCanSelectTile(bool bEnable);

protected:

	/** Our HUD class as a CSK HUD */
	UPROPERTY()
	ACSKHUD* CachedCSKHUD;

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

	/** Called to inform player that coin flip is taking place */
	void OnReadyForCoinFlip(); // Refactor

	/** Called by the game mode when transitioning to the board */
	void OnTransitionToBoard(); // Refactor

	/** Notify that the match has concluded */
	UFUNCTION(Client, Reliable)
	void Client_OnMatchFinished(bool bIsWinner);

public:

	/** Notify that we have collected resources during collection phase */
	UFUNCTION(Client, Reliable)
	void Client_OnCollectionPhaseResourcesTallied(FCollectionPhaseResourcesTally TalliedResources);

protected:

	/** Event for when this players collection phase resources has been tallied. This runs on the client and should
	ultimately call FinishCollectionTallyEvent when any local events have concluded (e.g. displaying the tallies) */
	UFUNCTION(BlueprintNativeEvent, Category = CSK)
	void OnCollectionResourcesTallied(int32 Gold, int32 Mana, bool bDeckReshuffled, TSubclassOf<USpellCard> SpellCard);

	/** Finishes the collection phase event. This must be called after collection resources tallied event */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void FinishCollectionSequenceEvent();

private:

	/** Handle transition to board client side */
	UFUNCTION(Client, Reliable)
	void Client_OnTransitionToBoard(); // TODO: refactor

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

	/** Enables/Disables this players quick effect ussage */
	void SetQuickEffectUsageEnabled(bool bEnable);

	/** Skips our quick effect counter selection if we are allowed to */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SkipQuickEffectSelection();

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
	UFUNCTION(BlueprintImplementableEvent, Category = CSK)
	void OnSelectionModeChanged(ECSKActionPhaseMode NewMode);

private:

	/** Notify the is action phase has been replicated */
	UFUNCTION()
	void OnRep_bIsActionPhase();

	/** Notify that remaining actions has been replicated */
	UFUNCTION()
	void OnRep_RemainingActions();

	/** Notify that can use quick effect has been replicated */
	UFUNCTION()
	void OnRep_bCanUseQuickEffect();

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

	/** If this player is allowed to select a counter spell */
	UPROPERTY(ReplicatedUsing = OnRep_bCanUseQuickEffect)
	uint32 bCanUseQuickEffect : 1;

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

	/** Notify that this player is able to counter an incoming spell cast */
	UFUNCTION(Client, Reliable)
	void Client_OnSelectCounterSpell(TSubclassOf<USpell> SpellToCounter, ATile* TargetTile);

	/** Notify that this players spell request is pending as the opposing player is selecting a counter */
	UFUNCTION(Client, Reliable)
	void Client_OnWaitForCounterSpell();

	/** Disable the ability to use the given action mode for the rest of this round.
	Get if no action remains (always returns false on client or if not in action phase) */
	bool DisableActionMode(ECSKActionPhaseMode ActionMode);

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

public:

	/** Get the list of towers this player can build */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetBuildableTowers(TArray<TSubclassOf<UTowerConstructionData>>& OutTowers) const;

	/** Get the list of spells this player can cast (in hand) */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetCastableSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const;

	/** Get the list of quick effect spells this player can cast (in hand) */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetCastableQuickEffectSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const;
};
