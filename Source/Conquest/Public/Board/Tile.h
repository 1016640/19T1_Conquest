// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

class ACSKPlayerController;
class ACSKPlayerState;
class IBoardPieceInterface;

/**
 * Actor for managing an individual tile on the board. Will track
 * any tower (including castles) that have been built upon it
 */
UCLASS()
class CONQUEST_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ATile();

public:

	#if WITH_EDITORONLY_DATA
	/** If this tile should be highlighted during PIE */
	UPROPERTY(EditInstanceOnly, Category = Debug)
	uint32 bHighlightTile : 1;
	#endif

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Set this tiles hex value on the board */
	FORCEINLINE void SetGridHexValue(const FIntVector& Hex) { GridHexIndex = Hex; }

	/** Get this tiles hex value on the board */
	FORCEINLINE const FIntVector& GetGridHexValue() const { return GridHexIndex; }

public:

	/** The element type of this tile */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Tile, meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 TileType;

	/** If this tile is a null tyle (no go zone) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	uint32 bIsNullTile : 1;

protected:

	/** This tiles hex value in the board managers grid. Is
	used to determine this tiles position amongst all others */
	UPROPERTY(VisibleAnywhere)
	FIntVector GridHexIndex;

public:

	/** Informs this tile that given player is now hovering over it */
	void StartHoveringTile(ACSKPlayerController* Controller);

	/** Informs this tile thay given player is no longer hovering over it */
	void EndHoveringTile(ACSKPlayerController* Controller);

protected:

	/** Event for when the player has started to hover over this tile. This event will only ever be called on the client */
	UFUNCTION(BlueprintImplementableEvent, Category = Tile, meta = (DisplayName="On Hover Start"))
	void BP_OnHoverStart(ACSKPlayerController* Controller);

	/** Event for when the player is no longer hovering over this tile. This event will only ever be called on the client */
	UFUNCTION(BlueprintImplementableEvent, Category = Tile, meta = (DisplayName = "On Hover End"))
	void BP_OnHoverEnd(ACSKPlayerController* Controller);

public:

	/** Set a new board piece to occupy this tile. Get if setting piece was successful */
	bool SetBoardPiece(AActor* BoardPiece);

	/** Clears the board piece currently occupying this tile. Get if a board piece was cleared */
	bool ClearBoardPiece();

protected:

	/** Event for when a new board piece has been placed on this tile */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board|Tiles", meta = (DisplayName="On Board Piece Set"))
	void BP_OnBoardPieceSet(AActor* NewBoardPiece);

	/** Event for when the occupant board piece has been cleared */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board|Tiles", meta = (DisplayName = "On Board Piece Cleared"))
	void BP_OnBoardPieceCleared(AActor* OldBoardPiece);

	/** Sets the board piece to occupy this tile */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_SetBoardPiece(AActor* BoardPiece);

	/** Clears the board piece occupying this tile */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClearBoardPiece();

public:

	/** If this tile is currently occupied. Null tiles are assumed to be occupied */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	virtual bool IsTileOccupied() const;

	/** If towers can be constructed on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	bool CanPlaceTowersOn() const;

	/** Get the player state for the player whose board piece is on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ACSKPlayerState* GetBoardPiecesOwner() const;

private:

	/** The board piece currently on this tile */
	UPROPERTY(Transient)
	TScriptInterface<IBoardPieceInterface> PieceOccupant;
};
