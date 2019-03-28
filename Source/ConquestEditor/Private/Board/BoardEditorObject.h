// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "SubclassOf.h"
#include "BoardEditorObject.generated.h"

class ABoardManager;
class ATile;
class FEdModeBoard;

/** Properties for a tile */
USTRUCT()
struct FBoardTileProperties
{
	GENERATED_BODY()

public:

	FBoardTileProperties()
		: TileType(1) // Fire
		, bIsNullTile(false)
	{

	}

public:

	/** The tile we are editing */
	TWeakObjectPtr<ATile> Tile;

	/** The tiles elemental type */
	UPROPERTY(EditAnywhere, Category = "Tile", meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 TileType;

	/** If this tile is a no go zone */
	UPROPERTY(EditAnywhere, Category = "Tile")
	uint8 bIsNullTile : 1;
};

/** 
 * Manages the state of the grid editors variables
 */
UCLASS(MinimalAPI)
class UBoardEditorObject : public UObject
{
	GENERATED_BODY()

public:

	UBoardEditorObject();

public:

	// Begin UObject Interface
	#if WITH_EDITOR	
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;	
	#endif
	// End UObject Interface

public:

	/** Set board managing this object */
	void SetEditorMode(FEdModeBoard* EdMode);

	/** Updates edit board properties to match board manager found by editor mode */
	void UpdateBoardEditProperties();

public:

	/** The amount of rows to generate for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 2, DisplayName="Rows"))
	int32 BoardRows;

	/** The amount of columns to generate for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 2, DisplayName = "Columns"))
	int32 BoardColumns;

	/** The size of each cell for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 10, DisplayName = "Hex Size"))
	float BoardHexSize;

	/** The origin for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 10, DislplayName = "Position"))
	FVector BoardOrigin;

	/** The type of tile to use when generating the board */
	UPROPERTY(EditAnywhere, Category = "Board", NoClear, meta = (DisplayName = "Tile Template"))
	TSubclassOf<ATile> BoardTileTemplate;

	/** Properties for the currently selected tile */
	UPROPERTY(EditAnywhere, Category = "Tile", meta = (ShowOnlyInnerProperties="true", BoardEdState = "Tile"))
	FBoardTileProperties TileProperties;

public:

	/** Notify from editor mode that editing has started */
	void NotifyEditingStart();

	/** Notify from editor that a board has been generated */
	void NotifyBoardGenerated();

public:

	/** The tile template used to generate the last grid. This will
	get modified by our editor mode when the board has been generated */
	TSubclassOf<ATile> LastBoardsTileType;

	/** If the board tile template has changed since the board has been generated. We
	should warn the user that generating the board will result in destruction of all tiles
	and that any changes applied to those tiles will be wiped */
	uint32 bWarnOfTileDifference : 1;

private:

	/** Editor mode managing us */
	FEdModeBoard* BoardEdMode;
};

