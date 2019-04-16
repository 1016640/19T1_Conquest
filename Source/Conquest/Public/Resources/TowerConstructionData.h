// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateBrush.h"
#include "TowerConstructionData.generated.h"

class ATower;

/**
 * Data relating to a specific tower players can build. Contains pricing
 * and additional information that can be displayed to the user.
 */
UCLASS(abstract, const, Blueprintable)
class CONQUEST_API UTowerConstructionData : public UObject
{
	GENERATED_BODY()
	
public:

	UTowerConstructionData();

public:

	/** Name of this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tower, meta = (DisplayName="Name"))
	FName TowerName;

	/** Description of this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tower, meta = (DisplayName = "Description", MultiLine = "true"))
	FText TowerDescription;

	/** Image of this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tower, meta = (DisplayName = "Icon"))
	FSlateBrush TowerIcon;

	/** Template actor for this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tower, meta = (DisplayName = "Class"))
	TSubclassOf<ATower> TowerClass;

	/** The cost of gold to build this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Cost, meta = (ClampMin = 0))
	int32 GoldCost;

	/** The cost of mana to build this tower */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Cost, meta = (ClampMin = 0))
	int32 ManaCost;
};