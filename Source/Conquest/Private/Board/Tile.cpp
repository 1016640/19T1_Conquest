// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);

	#if WITH_EDITORONLY_DATA
	// For debugging right now
	UBillboardComponent* Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(DummyRoot);
	Billboard->bIsEditorOnly = true;

	static ConstructorHelpers::FObjectFinder<UTexture2D> ActorTexture(TEXT("/Engine/EditorResources/S_Actor.S_Actor"));
	if (ActorTexture.Succeeded())
	{
		Billboard->SetSprite(ActorTexture.Object);
	}
	#endif

	// Fire tile type
	TileType = 1;
	bIsNullTile = false;
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
