// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Engine/AssetManager.h"
#include "ConquestAssetManager.generated.h"

/**
 * Asset manager specialized for CSK
 */
UCLASS()
class CONQUEST_API UConquestAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:

	/** Custom asset types */
	static const FPrimaryAssetType TowerBlueprintType;

public:

	/** Get conquest asset manager */
	static UConquestAssetManager& Get();
};
