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

	/** Get elemental types of this tile */
	FORCEINLINE uint8 GetElementalTypes() const { return TileType; }

	/** Get tiles that are connected to this one */
	FORCEINLINE const TArray<ATile*>& GetConnectedTiles() const { return ConnectedTiles; }

	/** Get if this tile is blocked (a no-go zone) */
	FORCEINLINE bool IsNullTile() const { return bIsNullTile; }


protected:

	/** The element type of this tile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tile, meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 TileType;

private:

	/** The tiles that can be reached from this tile */
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess="true"))
	TArray<ATile*> ConnectedTiles;

	/** If this tile is a null tyle (no go zone) */
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	uint32 bIsNullTile : 1;

public:

	/** If this tile has a building on it */
	UFUNCTION(BlueprintPure, Category = "Board|Tile")
	bool HasBuilding() const { return true; }
};
