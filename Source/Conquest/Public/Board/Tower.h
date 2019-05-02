// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "BoardPieceInterface.h"
#include "Tower.generated.h"

class ACSKPlayerController;
class UStaticMeshComponent;
class UTowerConstructionData;

/** Delegate for towers when they complete the build sequence */
DECLARE_MULTICAST_DELEGATE(FTowerBuildSequenceComplete);

/** 
 * Towers are board pieces that occupy a single tile and cannot move.
 */
UCLASS(abstract)
class CONQUEST_API ATower : public AActor, public IBoardPieceInterface
{
	GENERATED_BODY()
	
public:	

	ATower();

public:

	// Begin IBoardPiece Interface
	virtual void SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState) override;
	virtual ACSKPlayerState* GetBoardPieceOwnerPlayerState() const override { return OwnerPlayerState; }
	virtual void PlacedOnTile(ATile* Tile) override;
	virtual void OnHoverStart() override;
	virtual void OnHoverFinish() override;
	virtual UHealthComponent* GetHealthComponent() const override { return HealthTracker; }
	virtual void GetBoardPieceUIData(FBoardPieceUIData& OutUIData) const override;
	// End IBoardPiece Interface

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** The construction data about this tower */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = BoardPiece, AdvancedDisplay)
	const UTowerConstructionData* ConstructData;

public:

	/** Get mesh */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

private:

	/** The mesh that acts as the base of this tower (some towers might have more than mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Health component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthTracker;

public:

	/** Get the tile we are currently on */
	FORCEINLINE ATile* GetCachedTile() const { return CachedTile; }

protected:

	/** The player state of the player who owns this tower */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = BoardPiece)
	ACSKPlayerState* OwnerPlayerState;

private:

	/** The tile we are currently on, this is cached for quick access to it */
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, meta = (AllowPrivateAccess = "true"))
	ATile* CachedTile;

protected:

	/** Event for when the tile the castle is on has been hovered by the local player */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Hovered by Player"))
	void BP_OnHoveredByPlayer();

	/** Event for when the tile the castle is on is no longer hovered by the local player */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Unhovered By Player"))
	void BP_OnUnhoveredByPlayer();

public:

	/** Event for when the given player has built this tower. This should be used to
	grant or bind bonuses to the specific player. This function only runs on the server */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Tower, meta = (DisplayName = "On Built By Player"))
	void BP_OnBuiltByPlayer(ACSKPlayerController* Controller);

	/** Event for when this tower has been destroyed. This should be used to remove
	any granted bonuses from the specific player. This function only runs on the server */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Tower, meta = (DisplayName = "On Destroyed"))
	void BP_OnDestroyed(ACSKPlayerController* Controller);

	/** Event for when this tower should give resources to given player. This will only 
	run on the server and only when WantsCollectionPhaseEvent returns true */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = Tower, meta = (DisplayName = "Get Collection Phase Resources"))
	void BP_GetCollectionPhaseResources(const ACSKPlayerController* Controller, int32& OutGold, int32& OutMana, int32& OutSpellUses);

	/** Notify that this tower can begin to execute its end round action */
	void ExecuteEndRoundPhaseAction();

protected:

	/** Event for when we have started the build sequence. This is called on individual clients */
	UFUNCTION(BlueprintImplementableEvent, Category = Tower, meta = (DisplayName="On Start Build Sequence"))
	void BP_OnStartBuildSequence();

	/** Event for when we have finished the build sequence. This is called on individual clients */
	UFUNCTION(BlueprintImplementableEvent, Category = Tower, meta = (DisplayName = "On Finish Build Sequence"))
	void BP_OnFinishBuildSequence();

	/** Event for when this tower can execute its end round phase action. This will only
	run on the server and only when WantsEndRoundPhaseEvent returns true . When the event
	has concluded, FinishEndRoundAction should be called to signal this towers event is over */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = Tower)
	void StartEndRoundAction();

	/** Signals that this towers end round action has finished */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Tower)
	void FinishEndRoundAction();

private:

	/** Starts the building sequence */
	void StartBuildSequence();

	/** Ends the building sequence */
	void FinishBuildSequence();

public:

	/** If this tower is currently executing the build sequence */
	UFUNCTION(BlueprintPure, Category = Tower)
	bool IsExecutingBuildSequence() const { return bIsRunningBuildSequence; }

	/** If this tower is currently executing the end round action */
	UFUNCTION(BlueprintPure, Category = Tower)
	bool IsExecutingEndRoundAction() const { return bIsRunningEndRoundAction; }

public:

	/** Event for when build sequence has finish. This only gets executed on the server */
	FTowerBuildSequenceComplete OnBuildSequenceComplete;

private:

	/** If this tower is running its build sequence */
	UPROPERTY(VisibleInstanceOnly, Category = Tower)
	uint8 bIsRunningBuildSequence : 1;

	/** If this tower is running its end round action */
	UPROPERTY(VisibleInstanceOnly, Replicated, Category = Tower)
	uint8 bIsRunningEndRoundAction : 1;

public:

	/** Get if this tower is a legendary tower */
	FORCEINLINE bool IsLegendaryTower() const { return bIsLegendaryTower; }

	/** Get if this tower wants the collection phase event called.
	By default, will return bGivesCollectionPhaseResources */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Tower)
	bool WantsCollectionPhaseEvent() const;

	/** Get if this tower wants the end round phase event called.
	By default, will return bWantsActionDuringEndRoundPhase */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Tower)
	bool WantsEndRoundPhaseEvent() const;

	/** This towers priority during the end round phase */
	FORCEINLINE int32 GetEndRoundActionPriority() const { return EndRoundPhaseActionPriority; }

protected:

	/** If this tower is a legendary tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = BoardPiece)
	uint8 bIsLegendaryTower : 1;

	/** If this tower can give resources during the collection phase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoardPiece)
	uint8 bGivesCollectionPhaseResources : 1;

	/** If this tower wants to perform an action during the end round phase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoardPiece)
	uint8 bWantsActionDuringEndRoundPhase : 1;

	/** This towers priority over other towers who also have round end actions. This is used to determine
	which towers will execute their actions first. The lower the value, the higher the priority */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoardPiece, meta = (ClampMin = 0, EditCondition = "bWantsActionDuringEndRoundPhase"))
	int32 EndRoundPhaseActionPriority;

	/** How far this tower will push itself underground when initiating the build sequence.
	This is a scaler that will be applied to the meshes bound (specificallty the Z axis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = BoardPiece)
	float BuildSequenceUndergroundScale;

protected:

	/** Binds the custom tile selection to our owners player controller. This will
	only work during the end round action and will be automatically unbound. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = BoardPiece, meta = (BlueprintProtected="true"))
	void BindPlayerInput();

	/** Unbinds the custom tile selection from our owners player controller. This should be
	used when custom selection is no longer desired but can be left as it will be auto unbound */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = BoardPiece, meta = (BlueprintProtected="true"))
	void UnbindPlayerInput();

	/** If the player is allowed to select the given tile for end round action phase.
	This can run on both the owning players client and the server */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName="Can Select Tile for Action"))
	bool BP_CanSelectTileForAction(ATile* TargetTile);

	/** Called when player has selected the given tile for end round action. This only
	executes on the server and when CanSelectTileForAction has returned true */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = BoardPiece, meta = (DisplayName = "On Tile Selected for Action"))
	void BP_OnTileSelectedForAction(ATile* TargetTile);

private:

	/** Binds player input client side */
	UFUNCTION(Client, Reliable)
	void Client_BindPlayerInput(bool bBind);

private:

	/** If player input has been bound*/
	uint8 bIsInputBound : 1;
};
