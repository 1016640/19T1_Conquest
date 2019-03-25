// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEditorDetailCustomization.h"
#include "BoardEdMode.h"

#include "EditorModeManager.h"
#include "Widgets/Text/STextBlock.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyCustomizationHelpers.h"

FEdModeBoard* FBoardEditorDetailCustomization::GetEditorMode()
{
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	return static_cast<FEdModeBoard*>(ModeTools.GetActiveMode(FEdModeBoard::EM_Board));
}

#define LOCTEXT_NAMESPACE "BoardEditor.NewBoard"

TSharedRef<IDetailCustomization> FBoardEditorDetailCustomization_NewBoard::MakeInstance()
{
	return MakeShareable(new FBoardEditorDetailCustomization_NewBoard);
}

void FBoardEditorDetailCustomization_NewBoard::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& NewBoardCategory = DetailBuilder.EditCategory("New Board");
	NewBoardCategory.AddCustomRow(LOCTEXT("Test", "TESTING"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TestingTest", "This is a test"))
		];
}

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "BoardEditor.EditBoard"

#undef LOCTEXT_NAMESPACE