// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "PropertyCustomizationHelpers.h"

class FBoardToolkit;
class FEdModeBoard;

class SBoardEditor : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SBoardEditor) { }
	SLATE_END_ARGS()

public:

	virtual ~SBoardEditor() = default;

public:

	/** Constructs this widget */
	void Construct(const FArguments& InArgs, TSharedRef<FBoardToolkit> InParentToolkit);

private:

	/** If the given property should be visible. This will check the current editing state 
	to determine if new board variables or existing board variables should be shown */
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;

private:

	/** Get the current board editing mode */
	FEdModeBoard* GetEditorMode() const;

private:

	/** Details view to the board editor object */
	TSharedPtr<IDetailsView> BoardDetailsPanel;
};

