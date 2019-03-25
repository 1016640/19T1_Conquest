// Fill out your copyright notice in the Description page of Project Settings.

#include "ConquestEditorModule.h"
#include "ConquestEditor.h"

#include "PropertyEditorModule.h"
#include "Board/BoardEdMode.h"
#include "Board/BoardEditorDetailCustomization.h"

#define LOCTEXT_NAMESPACE "ConquestEditorModule"

class FConquestEditorModule : public IConquestEditorModule
{
public:

	virtual void StartupModule() override
	{
		RegisterEditorModes();
		RegisterDetailCustomizers();
	}

	virtual void ShutdownModule() override
	{
		UnregisterDetailCustomizers();
		UnregisterEditorModes();
	}

private:

	/** Registers editor modes */
	void RegisterEditorModes()
	{
		FEditorModeRegistry& EdModeRegistry = FEditorModeRegistry::Get();
		EdModeRegistry.RegisterMode<FEdModeBoard>(
			FEdModeBoard::EM_Board,
			LOCTEXT("BoardMode", "Conquest Board Editor"),
			FSlateIcon(),
			true);
	}

	/** Unregisters editor modes */
	void UnregisterEditorModes()
	{
		FEditorModeRegistry& EdModeRegistry = FEditorModeRegistry::Get();
		EdModeRegistry.UnregisterMode(FEdModeBoard::EM_Board);
	}

	/** Registers any custom detail customizers */
	void RegisterDetailCustomizers()
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.RegisterCustomClassLayout("BoardEditorObject", FOnGetDetailCustomizationInstance::CreateStatic(&FBoardEditorDetailCustomization_NewBoard::MakeInstance));
	}

	/** Unregisters any custom detail customizers */
	void UnregisterDetailCustomizers()
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomClassLayout("BoardEditorObject");
	}
};

IMPLEMENT_GAME_MODULE(FConquestEditorModule, ConquestEditor);

#undef LOCTEXT_NAMESPACE
