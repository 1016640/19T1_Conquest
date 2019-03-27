// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "SubclassOf.h"
#include "BoardEditorObject.generated.h"

class ABoardManager;
class ATile;
class FEdModeBoard;

USTRUCT()
struct FBoardTileObject
{
	GENERATED_BODY()

public:
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
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 2, DisplayName="Rows", BoardState="New"))
	int32 BoardRows;

	/** The amount of columns to generate for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 2, DisplayName = "Columns", BoardState = "New"))
	int32 BoardColumns;

	/** The size of each cell for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 10, DisplayName = "Hex Size", BoardState = "New"))
	float BoardHexSize;

	/** The origin for a new board */
	UPROPERTY(EditAnywhere, Category = "Board", meta = (ClampMin = 10, DislplayName = "Position", BoardState = "New"))
	FVector BoardOrigin;

	/** The type of tile to use when generating the board */
	UPROPERTY(EditAnywhere, Category = "Board", NoClear, meta = (DisplayName = "Tile Template", BoardState = "New"))
	TSubclassOf<ATile> BoardTileTemplate;

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

