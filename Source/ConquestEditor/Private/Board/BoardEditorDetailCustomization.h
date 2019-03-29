// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "Board/BoardTypes.h"
#include "PropertyHandle.h"

class FEdModeBoard;
class IDetailLayoutBuilder;

// TODO: Split up into different headers?

/**
 * Detail view customization for the board editor
 */
class FBoardEditorDetailCustomization : public IDetailCustomization
{
public:

	// Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override = 0;
	// End IDetailCustomization Interface

protected:

	/** Get the board editor mode */
	static FEdModeBoard* GetEditorMode();

protected:

	// Thanks to the Landscape module for these helper functions!
	
	/** Get value of a property */
	template <typename Type>
	static Type GetPropertyValue(TSharedRef<IPropertyHandle> PropertyHandle)
	{
		Type Value;
		if (PropertyHandle->GetValue(Value) == FPropertyAccess::Success)
		{
			return Value;
		}

		return Type();
	}

	/** Get optional version of a property */
	template <typename Type>
	static TOptional<Type> GetOptionalPropertyValue(TSharedRef<IPropertyHandle> PropertyHandle)
	{
		Type Value;
		if (PropertyHandle->GetValue(Value) == FPropertyAccess::Success)
		{
			return TOptional<Type>(Value);
		}

		return TOptional<Type>();
	}

	/** Sets the value of a property */
	template <typename Type>
	static void SetPropertyValue(Type Value, ETextCommit::Type CommitInfo, TSharedRef<IPropertyHandle> PropertyHandle)
	{
		ensure(PropertyHandle->SetValue(Value) == FPropertyAccess::Success);
	}
};

/** 
 * Detail view customization specific for struct properties
 */
class FBoardEditorStructCustomization : public IPropertyTypeCustomization
{
public:

	// Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) = 0;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) = 0;
	// End IPropertyTypeCustimization Interface

protected:

	/** Get the board editor mode */
	static FEdModeBoard* GetEditorMode();
};

/** 
 * Detail view for generating the board
 */
class FBoardEditorDetailCustomization_Board : public FBoardEditorDetailCustomization
{
public:

	/** Makes a new instance of this detail customizer */
	static TSharedRef<IDetailCustomization> MakeInstance();

public:

	// Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization Interface

protected:

	/** Get if tile template warning message should be displayed */
	static EVisibility GetVisibilityTileTemplateWarning();

protected:

	/** Notify that generate grid button has been pressed */
	FReply OnGenerateGridClicked();
};

/** 
 * Specialized property customization for editing properties of tiles
 */
class FBoardEditorStructCustomization_BoardTileProperties : public FBoardEditorStructCustomization
{
public:

	/** Makes a new instance of this property customizer */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

public:

	// Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	// End IPropertyTypeCustimization Interface

private:

	/** Get if tile hex value should be shown */
	static EVisibility GetTileHexValueVisibility();

	/** Get the tile hex value of the currently selected tile */
	static FText GetTileHexValueText();

	/** Get if given element type is set for selected tiles */
	static ECheckBoxState GetTilesIsElementSet(ECSKElementType ElementType);

	/** Set element enabled for all selected tiles */
	static void SetTilesElement(ECheckBoxState NewCheckedState, ECSKElementType ElementType);

	/** Get if selected tiles are null or not */
	static ECheckBoxState GetTilesIsNullTile();

	/** Set is null tile for all selected tiles */
	static void SetTilesIsNull(ECheckBoxState NewCheckedState);

	/** Get if player spawn buttons should be visible */
	static EVisibility GetVisibilitySetPlayerSpawns();

	/** Get if player spawn should be enabled */
	static bool GetTileCanSetSpawn(int32 Player);

	/** Set the player spawn for given player to current selected tile */
	static FReply SetBoardSpawnPoint(int32 Player);
};
