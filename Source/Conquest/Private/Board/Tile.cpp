// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->SetMobility(EComponentMobility::Static);

	// Fire tile type
	TileType = 1;
	bIsNullTile = false;
	GridHexIndex = FIntVector(-1);

	#if WITH_EDITORONLY_DATA
	bHighlightTile = false;
	#endif
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
