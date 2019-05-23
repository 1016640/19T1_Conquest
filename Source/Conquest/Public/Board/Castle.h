// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Pawn.h"
#include "BoardPieceInterface.h"
#include "Castle.generated.h"

class UFloatingPawnMovement;
class UStaticMeshComponent;

/**
 * Castles are towers that have the capability of moving. Castles are indirectly
 * controlled by players, but instead controlled by the CastleAIController
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACastle : public APawn, public IBoardPieceInterface
{
	GENERATED_BODY()

public:

	ACastle();

public:

	// Begin IBoardPiece Interface
	virtual void SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState) override;
	virtual ACSKPlayerState* GetBoardPieceOwnerPlayerState() const override { return OwnerPlayerState; }
	virtual void PlacedOnTile(ATile* Tile) override;
	virtual void RemovedOffTile() override;
	virtual void OnHoverStart() override;
	virtual void OnHoverFinish() override;
	virtual UHealthComponent* GetHealthComponent() const override { return HealthTracker; }
	virtual void GetBoardPieceUIData(FBoardPieceUIData& OutUIData) const override;
	// End IBoardPiece Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Get static mesh component */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** Get pawn movement component */
	FORCEINLINE UFloatingPawnMovement* GetPawnMovement() const { return PawnMovement; }

private:

	/** Animated skeletal mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	/** Basic Movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UFloatingPawnMovement* PawnMovement;

	/** Health component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthTracker;

protected:

	/** Event for when the castle has been placed on given tile */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Placed on Tile"))
	void BP_OnPlacedOnTile(ATile* Tile);

	/** Event for when the castle has moved off of given tile */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Removed from Tile"))
	void BP_OnRemovedFromTile(ATile* Tile);

	/** Event for when the tile the castle is on has been hovered by the local player */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Hovered by Player"))
	void BP_OnHoveredByPlayer();

	/** Event for when the tile the castle is on is no longer hovered by the local player */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Unhovered By Player"))
	void BP_OnUnhoveredByPlayer();

public:

	/** Get the player who owns this castle */
	FORCEINLINE ACSKPlayerState* GetOwnerPlayerState() const { return OwnerPlayerState; }

	/** Get the tile we are currently on */
	FORCEINLINE ATile* GetCachedTile() const { return CachedTile; }

protected:

	/** The player state of the player who owns this castle */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = BoardPiece)
	ACSKPlayerState* OwnerPlayerState;

private:

	/** The tile we are currently on, this is cached for quick access to it */
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, meta = (AllowPrivateAccess = "true"))
	ATile* CachedTile;
};
