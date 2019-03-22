// Fill out your copyright notice in the Description page of Project Settings.

#include "ConquestEditorModule.h"
#include "ConquestEditor.h"

#include "Board/BoardEdMode.h"

#define LOCTEXT_NAMESPACE "ConquestEditorModule"

class FConquestEditorModule : public IConquestEditorModule
{
public:

	virtual void StartupModule() override
	{
		RegisterBoardEditor();
	}

	virtual void ShutdownModule() override
	{
		UnregisterBoardEditor();
	}

private:

	/** Registers board editor mode */
	void RegisterBoardEditor()
	{
		FEditorModeRegistry& EdModeRegistry = FEditorModeRegistry::Get();
		EdModeRegistry.RegisterMode<FEdModeBoard>(
			FEdModeBoard::EM_BoardModeID,
			LOCTEXT("BoardMode", "Conquest Board Editor"),
			FSlateIcon(),
			true);
	}

	/** Unregisters board editor mode */
	void UnregisterBoardEditor()
	{
		FEditorModeRegistry& EdModeRegistry = FEditorModeRegistry::Get();
		EdModeRegistry.UnregisterMode(FEdModeBoard::EM_BoardModeID);
	}
};

IMPLEMENT_GAME_MODULE(FConquestEditorModule, ConquestEditor);

#undef LOCTEXT_NAMESPACE
