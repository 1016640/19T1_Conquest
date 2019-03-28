// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tile, meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 TileType;

	/** If this tile is a null tyle (no go zone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	uint32 bIsNullTile : 1;

protected:

	/** This tiles hex value in the board managers grid. Is
	used to determine this tiles position amongst all others */
	UPROPERTY(VisibleAnywhere)
	FIntVector GridHexIndex;

public:

	/** If this tile has a building on it */
	UFUNCTION(BlueprintPure, Category = "Board|Tile")
	bool HasBuilding() const { return true; }

public:

};
