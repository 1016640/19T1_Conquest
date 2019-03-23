// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "BoardEditorObject.generated.h"

class ABoardManager;
class FEdModeBoard;

/** 
 * Manages the state of the grid editors variables
 */
UCLASS()
class UBoardEditorObject : public UObject
{
	GENERATED_BODY()

public:

	UBoardEditorObject();

public:

	/** The amount of rows to generate for a new board */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Settings", meta = (ClampMin = 2, DisplayName="Rows", BoardState="New"))
	int32 New_BoardRows;

	/** The amount of columns to generate for a new board */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Settings", meta = (ClampMin = 2, DisplayName = "Columns", BoardState = "New"))
	int32 New_BoardColumns;

	/** The size of each cell for a new board */
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Settings", meta = (ClampMin = 10, DisplayName = "Hex Size", BoardState = "New"))
	float New_BoardHexSize;

private:

	/** Editor mode managing us */
	FEdModeBoard* EdMode;
};

