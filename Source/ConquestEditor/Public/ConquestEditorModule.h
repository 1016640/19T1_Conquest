// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IConquestEditorModule : public IModuleInterface
{
public:

	/** Get conquest module */
	static inline IConquestEditorModule& GetConquestModule()
	{
		return FModuleManager::LoadModuleChecked<IConquestEditorModule>("ConquestEditor");
	}
};

