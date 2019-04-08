// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "BoardPieceInterface.h"
#include "Tower.generated.h"

class UStaticMeshComponent;

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
	// End IBoardPiece Interface

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Get mesh */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

private:

	/** The mesh that acts as the base of this tower (some towers might have more than mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

protected:

	/** The player state of the player who owns this tower */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = BoardPiece)
	ACSKPlayerState* OwnerPlayerState;

protected:

	/** Event for when we have started the build sequence. This is called on individual clients */
	UFUNCTION(BlueprintImplementableEvent, Category = Tower, meta = (DisplayName="On Start Build Sequence"))
	void BP_OnStartBuildSequence();

	/** Event for when we have finished the build sequence. This is called on individual clients */
	UFUNCTION(BlueprintImplementableEvent, Category = Tower, meta = (DisplayName = "On Finish Build Sequence"))
	void BP_OnFinishBuildSequence();

private:

	/** Starts the building sequence */
	void StartBuildSequence();

	/** Ends the building sequence */
	void FinishBuildSequence();

public:

	/** Event for when build sequence has finish. This only gets executed on the server */
	FTowerBuildSequenceComplete OnBuildSequenceComplete;

private:

	/** The tile we are currently on, this is cached for quick access to it */
	UPROPERTY(Transient, DuplicateTransient)
	ATile* CachedTile;

public:

	/** Get if this tower is a legendary tower */
	UFUNCTION(BlueprintPure, Category = "Board|Towers")
	bool IsLegendaryTower() const { return bIsLegendaryTower; }

protected:

	/** If this tower is a legendary tower */
	UPROPERTY(EditDefaultsOnly, Category = BoardPiece)
	uint8 bIsLegendaryTower : 1;
};
