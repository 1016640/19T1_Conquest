// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEditorDetailCustomization.h"
#include "BoardEditorObject.h"
#include "BoardEdMode.h"

#include "EditorModeManager.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
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

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBoardEditorDetailCustomization_NewBoard::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& NewBoardCategory = DetailBuilder.EditCategory("New Board");

	// Description
	NewBoardCategory.AddCustomRow(LOCTEXT("NewBoardDescRow", "Description"))
	[
		SNew(SBorder)
		.Padding(FMargin(2.f))
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NewBoardDescription", "Tool for creating a new board.\nThe board is created using a rectangular Hex Grid that can be customized in terms of Rows, Columns and Hex size"))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
			.Font(DetailBuilder.GetDetailFont())
		]
	];

	TSharedRef<IPropertyHandle> PropertyHandle_Rows = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, New_BoardRows));

	NewBoardCategory.InitiallyCollapsed(false);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "BoardEditor.EditBoard"

#undef LOCTEXT_NAMESPACE