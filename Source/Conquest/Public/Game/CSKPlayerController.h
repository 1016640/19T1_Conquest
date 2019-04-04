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

	/** Sets the current tile we are hovering over as selected */
	void SelectTile();

protected:

	/** The tile we are currently hovering over (only valid on the client) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* HoveredTile;

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
	void OnTransitionToBoard(int32 PlayerID);

private:

	/** Handle transition to board client side */
	UFUNCTION(Client, Reliable)
	void Client_OnTransitionToBoard();

public:

	/** Starts this players action phase */
	void StartActionPhase();

	/** Enters the given action mode */
	//UFUNCTION(BlueprintCallable, Category = CSK)
	//void SetActionMode(ECSKActionPhaseMode NewMode);

public:

	/** If this castle is allowed to request a castle move */
	bool CanRequestMoveAction() const;

	/** Get if this player cen enter given action phase state */
	//UFUNCTION(BlueprintPure, Category = CSK)
	//bool CanEnterActionState(ECSKPlayerActionPhaseState ActionState) const;

private:

	/** Notify that remaining actions has been replicated */
	UFUNCTION()
	void OnRep_RemainingActions() { }

protected:

	/** If it is our action phase */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	uint32 bIsActionPhase : 1;

	/** The current action mode we are in */
	UPROPERTY(BlueprintReadOnly, Category = CSK)
	ECSKActionPhaseMode SelectedAction;

	/** The actions we can still during our action phase */
	UPROPERTY(ReplicatedUsing = OnRep_RemainingActions)
	ECSKActionPhaseMode RemainingActions;

public:

	/** Notify from the game mode that our move request was accepted */
	void ConfirmedTravelToTile(const FBoardPath& BoardPath);

	/** Notify from the game state to track castle in motion */
	UFUNCTION(Client, Reliable)
	void Client_FocusCastleDuringMovement(ACastle* MovingCastle);

	/** Notify from the game state that we can stop focusing castle */
	UFUNCTION(Client, Reliable)
	void Client_StopFocusingCastle();

protected:

	/** Attempts to move our castle to given tile */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestTravelToTile(ATile* Tile);
};
