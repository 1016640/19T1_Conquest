// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardManager.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"

ABoardManager::ABoardManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	bNetLoadOnClient = true;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);

	#if WITH_EDITORONLY_DATA
	UBillboardComponent* Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(DummyRoot);
	Billboard->bIsEditorOnly = true;

	static ConstructorHelpers::FObjectFinder<UTexture2D> ActorTexture(TEXT("/Engine/EditorResources/S_Actor.S_Actor"));
	if (ActorTexture.Succeeded())
	{
		Billboard->SetSprite(ActorTexture.Object);
	}
	#endif

	GridDimensions = FIntPoint(0, 0);
	GridHexSize = 0.f;
	#if WITH_EDITORONLY_DATA
	GridTileTemplate = nullptr;
	#endif

}

void ABoardManager::DestroyBoard()
{
	HexGrid.ClearGrid();
	Destroy();
}

#if WITH_EDITOR
void ABoardManager::InitBoard(const FBoardInitData& InitData)
{
	if (!ensure(InitData.IsValid()))
	{
		return;
	}

	if (true)//GridTileTemplate != InitData.GetTileTemplate())
	{
		HexGrid.ClearGrid();
	}

	GridDimensions = InitData.Dimensions;
	GridHexSize = InitData.HexSize;
	GridTileTemplate = InitData.GetTileTemplate();
	SetActorLocationAndRotation(InitData.Origin, InitData.Rotation);

	// Variables for the tile predicate to use
	TSubclassOf<ATile> TileTemplate = GridTileTemplate;
	FVector GridSize(GridHexSize, GridHexSize, 0.f);
	int32 CellID = 0;

	// Predicate that will spawn a tile specified by the hex grid during its generation
	auto TilePredicate = [this, GridSize, TileTemplate, &CellID](const FHexGrid::FHex& Hex, int32 Row, int32 Column)->ATile*
	{
		UWorld* World = this->GetWorld();
		if (World)
		{
			FVector TileLocation = FHexGrid::ConvertHexToWorld(Hex, this->GetActorLocation(), GridSize);
			FString TileID = FString("BoardTile") + FString::FromInt(CellID++);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			//SpawnParams.Name = FName(*TileID); // This is causing weird issues when rebuilding (assuming this is due to these actors potentially existing at this point)
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			// Init tile
			ATile* Tile = World->SpawnActor<ATile>(TileTemplate, TileLocation, FRotator::ZeroRotator, SpawnParams);
			if (Tile)
			{
				// Easy identifier for in editor work (TODO: Have hex be printed instead of ID)
				Tile->SetActorLabel("BoardTile");

				// Attach so tiles move when board does
				Tile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				
				// Init tile
				Tile->SetGridHexValue(Hex);
			}

			return Tile;
		}

		return nullptr;
	};

	// Generate the grid
	HexGrid.GenerateGrid(GridDimensions.X, GridDimensions.Y, TilePredicate);
}
#endif
