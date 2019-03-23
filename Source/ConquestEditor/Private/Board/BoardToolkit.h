// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "Toolkits/BaseToolkit.h"

class FEdModeBoard;

/** 
 * Toolkit that supports board editor mode
 */
class FBoardToolkit : public FModeToolkit
{
public:

	FBoardToolkit(FEdModeBoard* EdMode);
	virtual ~FBoardToolkit() = default;

public:

	// Begin IToolkit Interface 
	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	// End IToolkit Interface

private:

	/** The editor mode we belong to */
	FEdModeBoard* BoardEdMode;
};

