// Fill out your copyright notice in the Description page of Project Settings.

#include "ConquestAssetManager.h"
#include "Engine/Engine.h"

const FPrimaryAssetType UConquestAssetManager::TowerBlueprintType("TowerBlueprint");

UConquestAssetManager& UConquestAssetManager::Get()
{
	UConquestAssetManager* AssetManager = Cast<UConquestAssetManager>(GEngine->AssetManager);
	if (AssetManager)
	{
		return *AssetManager;
	}
	else
	{
		UE_LOG(LogConquest, Fatal, TEXT("UConquestAssetManager::Get: Failed to get Conquest Asset Manager. "
			"Conquest Asset Manager must be specified in defaultengine.ini."));
		return *NewObject<UConquestAssetManager>();
	}
}
