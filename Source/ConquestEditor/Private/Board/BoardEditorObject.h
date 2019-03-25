// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "BoardEditorObject.generated.h"

class ABoardManager;
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
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;	
	#endif
	// End UObject Interface

public:

	/** Set board managing this object */
	void SetEditorMode(FEdModeBoard* EdMode);

	/** Updates edit board properties to match board manager found by editor mode */
	void UpdateBoardEditProperties();

public:

	/** The amount of rows to generate for a new board */
	UPROPERTY(EditAnywhere, Category = "New Board", meta = (ClampMin = 2, DisplayName="Rows", BoardState="New"))
	int32 New_BoardRows;

	/** The amount of columns to generate for a new board */
	UPROPERTY(EditAnywhere, Category = "New Board", meta = (ClampMin = 2, DisplayName = "Columns", BoardState = "New"))
	int32 New_BoardColumns;

	/** The size of each cell for a new board */
	UPROPERTY(EditAnywhere, Category = "New Board", meta = (ClampMin = 10, DisplayName = "Hex Size", BoardState = "New"))
	float New_BoardHexSize;

	/** The origin for a new board */
	UPROPERTY(EditAnywhere, Category = "New Board", meta = (ClampMin = 10, DislplayName = "Position", BoardState = "New"))
	FVector New_BoardOrigin;

	/** The amount of rows to regenerate the existing board with */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Edit Board", meta = (ClampMin = 2, DisplayName = "Rows", BoardState = "Edit"))
	int32 Edit_BoardRows;

	/** The amount of columns to regenerate the existing board with */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Edit Board", meta = (ClampMin = 2, DisplayName = "Columns", BoardState = "Edit"))
	int32 Edit_BoardColumns;

	/** The size of each cell for the existing board */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Edit Board", meta = (ClampMin = 10, DisplayName = "Hex Size", BoardState = "Edit"))
	float Edit_BoardHexSize;

private:

	/** Editor mode managing us */
	FEdModeBoard* BoardEdMode;
};

