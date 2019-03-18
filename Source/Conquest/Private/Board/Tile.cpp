// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
