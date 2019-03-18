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

protected:

	/** The element type of this tile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tile, meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 TileType;
};
