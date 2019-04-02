// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

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

	/** If this tile is currently occupied. Null tiles are assumed to be occupied */
	UFUNCTION(BlueprintPure, Category = "Board")
	virtual bool IsTileOccupied() const;

private:

	// TODO: Save on client via RPC
	/** The board piece currently on this tile */
	UPROPERTY(Transient)
	TScriptInterface<IBoardPieceInterface> PieceOccupant;
};
