// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "Toolkits/BaseToolkit.h"
#include "BoardEdMode.h"

class SBoardEditor;

/** 
 * Toolkit that supports board editor mode
 */
class FBoardToolkit : public FModeToolkit
{
public:

	FBoardToolkit() = default;
	virtual ~FBoardToolkit() = default;

public:

	// Begin IToolkit Interface 
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;
	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FEdModeBoard* GetEditorMode() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;
	// End IToolkit Interface

public:

	/** Notify that the board editing state has changed */
	void NotifyEditingStateChanged();

private:

	/** The widget to edit the board with */
	TSharedPtr<SBoardEditor> BoardWidget;
};

