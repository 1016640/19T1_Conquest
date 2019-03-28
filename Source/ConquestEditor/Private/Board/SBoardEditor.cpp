// Fill out your copyright notice in the Description page of Project Settings.

#include "SBoardEditor.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"

#include "BoardEdMode.h"
#include "EditorModeManager.h"
#include "ModuleManager.h"
#include "PropertyEditorModule.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBoardEditor::Construct(const FArguments& InArgs, TSharedRef<FBoardToolkit> InParentToolkit)
{
	// Create a details view for the board settings
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		const FDetailsViewArgs::ENameAreaSettings NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
		BoardDetailsPanel = PropertyEditorModule.CreateDetailView(FDetailsViewArgs(false, false, false, NameAreaSettings));
		BoardDetailsPanel->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SBoardEditor::GetIsPropertyVisible));

		// Initialize details panel
		FEdModeBoard* BoardEdMode = GetEditorMode();
		if (BoardEdMode)
		{
			BoardDetailsPanel->SetObject(BoardEdMode->GetBoardSettings());
		}
	}

	// TODO: Fixup
	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.Padding(0)
		[
			BoardDetailsPanel.ToSharedRef()
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool SBoardEditor::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	const UProperty& Property = PropertyAndParent.Property;

	FEdModeBoard* BoardEdMode = GetEditorMode();
	if (BoardEdMode)
	{
		// Only display this property is it relates to the board editing state
		if (Property.HasMetaData("BoardEdState"))
		{
			FString RequiredState;
			switch (BoardEdMode->GetCurrentEditingState())
			{
				case EBoardEditingState::GenerateBoard:
				{
					RequiredState = "New";
					break;
				}
				case EBoardEditingState::EditBoard:
				{
					RequiredState = "Edit";
					break;
				}
				case EBoardEditingState::TileSelected:
				{
					RequiredState = "Tile";
					break;
				}
			}

			return Property.GetMetaData("BoardEdState").Contains(RequiredState);
		}

		// If property does not specify a board state, assume it can be used with any
		return true;
	}

	return false;
}

void SBoardEditor::RefreshDetailsPanel() const
{
	FEdModeBoard* BoardEdMode = GetEditorMode();
	if (BoardEdMode)
	{
		BoardDetailsPanel->SetObject(BoardEdMode->GetBoardSettings(), true);
	}
}

FEdModeBoard* SBoardEditor::GetEditorMode() const
{
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	return static_cast<FEdModeBoard*>(ModeTools.GetActiveMode(FEdModeBoard::EM_Board));
}
