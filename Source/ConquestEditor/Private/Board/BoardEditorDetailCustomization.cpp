// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEditorDetailCustomization.h"
#include "BoardEditorObject.h"
#include "BoardEdMode.h"

#include "EditorModeManager.h"
#include "SlateOptMacros.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SVectorInputBox.h"
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
	FEdModeBoard* BoardEdMode = GetEditorMode();
	if (BoardEdMode->IsEditingBoard())
	{
		return;
	}

	IDetailCategoryBuilder& BoardCategory = DetailBuilder.EditCategory("Board");

	// Description
	{
		BoardCategory.AddCustomRow(LOCTEXT("BoardDescRow", "Description"))
		[
			SNew(SBorder)
			.Padding(FMargin(5.f))
			.HAlign(HAlign_Fill)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BoardDescription", "Tool for creating a new board.\nThe board is created using a rectangular Hex Grid that can be customized in terms of Rows, Columns and Hex Size."))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
				.Font(DetailBuilder.GetDetailFont())
			]
		];
	}

	// Adjust rows
	{
		TSharedRef<IPropertyHandle> PropertyHandle_Rows = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardRows));
		BoardCategory.AddProperty(PropertyHandle_Rows)
		.CustomWidget()
		.NameContent()
		[
			PropertyHandle_Rows->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SNumericEntryBox<int32>)
			.MinValue(2)
			.Delta(1)
			.Value_Static(&GetOptionalPropertyValue<int32>, PropertyHandle_Rows)
			.OnValueCommitted_Static(&SetPropertyValue<int32>, PropertyHandle_Rows)
		];
	}

	// Adjust columns
	{
		TSharedRef<IPropertyHandle> PropertyHandle_Columns = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardColumns));
		BoardCategory.AddProperty(PropertyHandle_Columns)
		.CustomWidget()
		.NameContent()
		[
			PropertyHandle_Columns->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SNumericEntryBox<int32>)		
			.MinValue(2)
			.Delta(1)
			.Value_Static(&GetOptionalPropertyValue<int32>, PropertyHandle_Columns)
			.OnValueCommitted_Static(&SetPropertyValue<int32>, PropertyHandle_Columns)
		];
	}

	// Adjust hex size
	{
		TSharedRef<IPropertyHandle> PropertyHandle_HexSize = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardHexSize));
		BoardCategory.AddProperty(PropertyHandle_HexSize)
		.CustomWidget()
		.NameContent()
		[
			PropertyHandle_HexSize->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SNumericEntryBox<float>)		
			.MinValue(10.f)
			.Delta(5.f)
			.Value_Static(&GetOptionalPropertyValue<float>, PropertyHandle_HexSize)
			.OnValueCommitted_Static(&SetPropertyValue<float>, PropertyHandle_HexSize)
		];
	}

	// Adjust origin
	{
		TSharedRef<IPropertyHandle> PropertyHandle_Origin = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardOrigin));
		TSharedRef<IPropertyHandle> PropertyHandle_OriginX = PropertyHandle_Origin->GetChildHandle("X").ToSharedRef();
		TSharedRef<IPropertyHandle> PropertyHandle_OriginY = PropertyHandle_Origin->GetChildHandle("Y").ToSharedRef();
		TSharedRef<IPropertyHandle> PropertyHandle_OriginZ = PropertyHandle_Origin->GetChildHandle("Z").ToSharedRef();
		BoardCategory.AddProperty(PropertyHandle_Origin)
		.CustomWidget()
		.NameContent()
		[
			PropertyHandle_Origin->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(450.f)
		.MaxDesiredWidth(450.f)
		[
			SNew(SVectorInputBox)
			.Font(DetailBuilder.GetDetailFont())
			.bColorAxisLabels(true)
			.AllowSpin(false)
			.X_Static(&GetOptionalPropertyValue<float>, PropertyHandle_OriginX)
			.Y_Static(&GetOptionalPropertyValue<float>, PropertyHandle_OriginY)
			.Z_Static(&GetOptionalPropertyValue<float>, PropertyHandle_OriginZ)
			.OnXCommitted_Static(&SetPropertyValue<float>, PropertyHandle_OriginX)
			.OnYCommitted_Static(&SetPropertyValue<float>, PropertyHandle_OriginY)
			.OnZCommitted_Static(&SetPropertyValue<float>, PropertyHandle_OriginZ)
		];
	}

	// Change tile type
	{
		TSharedRef<IPropertyHandle> PropertyHandle_TileTemplate = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardTileTemplate));
		BoardCategory.AddProperty(PropertyHandle_TileTemplate);

		BoardCategory.AddCustomRow(LOCTEXT("BoardTileWarnRow", "Tile Template Warning"))
		[
			SNew(SBorder)
			.Padding(FMargin(5.f))
			.HAlign(HAlign_Fill)
			.Visibility_Static(&GetVisibilityTileTemplateWarning)
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFont())
				.Text(LOCTEXT("BoardTileTemplateWarning", "Changing the type of tile will force complete re-creation of the board. This will wipe all changes made to individual tiles!"))
				.ColorAndOpacity(FLinearColor::Red)
				.AutoWrapText(true)
			]
		];
	}

	// Generate button
	{
		BoardCategory.AddCustomRow(FText::GetEmpty())
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.OnClicked(this, &FBoardEditorDetailCustomization_NewBoard::OnGenerateGridClicked)
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFontBold())
				.Text(LOCTEXT("GenerateGrid", "Generate"))
				.Justification(ETextJustify::Center)
			]
		];
	}

	BoardCategory.InitiallyCollapsed(false);
}

EVisibility FBoardEditorDetailCustomization_NewBoard::GetVisibilityTileTemplateWarning()
{
	FEdModeBoard* BoardEdMode = GetEditorMode();
	if (BoardEdMode)
	{
		const UBoardEditorObject* BoardSettings = BoardEdMode->GetBoardSettings();
		return BoardSettings->bWarnOfTileDifference ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

FReply FBoardEditorDetailCustomization_NewBoard::OnGenerateGridClicked()
{
	FEdModeBoard* BoardEdMode = GetEditorMode();
	if (BoardEdMode)
	{
		BoardEdMode->GenerateBoard();
	}

	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "BoardEditor.EditBoard"

#undef LOCTEXT_NAMESPACE