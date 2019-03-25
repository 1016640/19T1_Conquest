// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardToolkit.h"
#include "SBoardEditor.h"

#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "FEdModeBoardToolkit"

void FBoardToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	BoardWidget = SNew(SBoardEditor, SharedThis(this));

	FModeToolkit::Init(InitToolkitHost);
}

FText FBoardToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BoardEditorToolkitBaseName", "BoardEditorToolkit");
}

FName FBoardToolkit::GetToolkitFName() const
{
	return TEXT("BoardEditorToolkit");
}

FEdModeBoard* FBoardToolkit::GetEditorMode() const
{
	return (FEdModeBoard*)GLevelEditorModeTools().GetActiveMode(FEdModeBoard::EM_Board);
}

TSharedPtr<SWidget> FBoardToolkit::GetInlineContent() const
{
	return BoardWidget.ToSharedRef();
}

void FBoardToolkit::NotifyEditingStateChanged()
{
	BoardWidget->RefreshDetailsPanel();
}

#undef LOCTEXT_NAMESPACE