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
	UPROPERTY(Transient, DuplicateTransient, Replicated)
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

protected:

	/** The tile we are currently hovering over (only valid on the client) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* HoveredTile;

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

public:

	/** Called to inform player that coin flip is taking place */
	void OnReadyForCoinFlip();

	/** Called by the game mode when transitioning to the board */
	void OnTransitionToBoard();

private:

	/** Handle transition to board client side */
	UFUNCTION(Client, Reliable)
	void Client_OnTransitionToBoard();

public:

	/** Enables/Disables this players action phase */
	void SetActionPhaseEnabled(bool bEnabled);

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

	/** Get if this player cen enter given action mode */
	UFUNCTION(BlueprintPure, Category = CSK)
	bool CanEnterActionMode(ECSKActionPhaseMode ActionMode) const;

	/** If this player is allowed to request a castle move */
	bool CanRequestCastleMoveAction() const;

	/** If this player is allowed to request a tower construction */
	bool CanRequestBuildTowerAction() const;

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
	void Client_OnTowerBuildRequestConfirmed(ATower* NewTower);

	/** Disable the ability to use the given action mode for the rest of this round.
	Get if no action remains (always returns false on client or if not in action phase) */
	bool DisableActionMode(ECSKActionPhaseMode ActionMode);

protected:

	/** Makes a request to move our castle towards the goal tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCastleMoveAction(ATile* Goal);
};
