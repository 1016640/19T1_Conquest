// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class IConquestModule : public IModuleInterface
{
public:

	/** Get conquest module */
	static inline IConquestModule& GetConquestModule()
	{
		return FModuleManager::LoadModuleChecked<IConquestModule>("ConquestModule");
	}
};

