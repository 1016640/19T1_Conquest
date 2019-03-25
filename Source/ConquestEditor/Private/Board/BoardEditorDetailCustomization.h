// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FEdModeBoard;
class IDetailLayoutBuilder;

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
 * Detail view for creating a new board 
 */
class FBoardEditorDetailCustomization_NewBoard : public FBoardEditorDetailCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

public:

	// Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End IDetailCustomization Interface

protected:

	/** Notify that generate grid button has been pressed */
	FReply OnGenerateGridClicked();
};
