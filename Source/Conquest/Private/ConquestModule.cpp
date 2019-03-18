// Fill out your copyright notice in the Description page of Project Settings.

#include "ConquestModule.h"

class FConquestModule : public IConquestModule
{
public:

	virtual bool IsGameModule() const
	{
		return true;
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FConquestModule, Conquest, "Conquest");
