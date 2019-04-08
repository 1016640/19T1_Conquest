// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerData.h"
#include "Tower.h"

UTowerConstructionData::UTowerConstructionData()
{
	TowerName = TEXT("Tower");
	TowerClass = ATower::StaticClass();
	GoldCost = 5;
	ManaCost = 0;
}