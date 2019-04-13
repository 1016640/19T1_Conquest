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

protected:

	/** Our HUD class as a CSK HUD */
	UPROPERTY()
	ACSKHUD* CachedCSKHUD;

private:

	/** Attempts to perform an action using the current hovered tile */
	void SelectTile();

	/** Resets our camera to focus on our castle */
	void ResetCamera();

	/** Sets whethere we can select tiles */
	void SetCanSelectTile(bool bEnable);

protected:

	/** The tile we are currently hovering over (only valid on the client) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* HoveredTile;

	/** The tile we have selected (clicked on) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* SelectedTile;

	/** If we are accepting input via select tile (only valid on the client) */
	UPROPERTY(Transient)
	uint32 bCanSelectTile : 1;

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

	/** Notify that an action phase spell cast has been confirmed */
	UFUNCTION(Client, Reliable)
	void Client_OnCastSpellRequestConfirmed(ATile* TargetTile);

	/** Notify that an action phase spell cast has finished */
	UFUNCTION(Client, Reliable)
	void Client_OnCastSpellRequestFinished();

	/** Disable the ability to use the given action mode for the rest of this round.
	Get if no action remains (always returns false on client or if not in action phase) */
	bool DisableActionMode(ECSKActionPhaseMode ActionMode);

public:

	/** Builds the request tower at the currently selected tile. 
	This will only work if running on local players client */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void BuildTowerAtTile(TSubclassOf<UTowerConstructionData> TowerConstructData);

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

	/** Makes a request to cast a spell at given tile (with additional mana cost */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastSpellAction(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana);

public:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UTowerConstructionData> TestTowerTemplate;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USpellCard> TestSpellCardTemplate;

	UPROPERTY(EditAnywhere)
	int32 TestSpellCardSpellIndex = 0;

public:

	/** Get the list of towers this player can build */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetBuildableTowers(TArray<TSubclassOf<UTowerConstructionData>>& OutTowers) const;

	/** Get the list of spells this player can cast (in hand) */
	UFUNCTION(BlueprintPure, Category = CSK)
	void GetCastableSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const;
};
