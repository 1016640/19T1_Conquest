// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardToolkit.h"
#include "BoardEdMode.h"

#define LOCTEXT_NAMESPACE "FEdModeBoardToolkit"

FBoardToolkit::FBoardToolkit(FEdModeBoard* EdMode)
	: BoardEdMode(EdMode)
{
	
}

FText FBoardToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BoardEditorToolkitBaseName", "BoardEditorToolkit");
}

FName FBoardToolkit::GetToolkitFName() const
{
	return TEXT("BoardEditorToolkit");
}

FEdMode* FBoardToolkit::GetEditorMode() const
{
	return BoardEdMode;
}

#undef LOCTEXT_NAMESPACE