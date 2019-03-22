// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEdModeToolkit.h"
#include "BoardEdMode.h"

#define LOCTEXT_NAMESPACE "FEdModeBoardToolkit"

FEdModeBoardToolkit::FEdModeBoardToolkit(FEdModeBoard* EdMode)
	: BoardEdMode(EdMode)
{
	
}

FText FEdModeBoardToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BoardEditorToolkitBaseName", "BoardEditorToolkit");
}

FName FEdModeBoardToolkit::GetToolkitFName() const
{
	return TEXT("BoardEditorToolkit");
}

FEdMode* FEdModeBoardToolkit::GetEditorMode() const
{
	return BoardEdMode;
}

#undef LOCTEXT_NAMESPACE