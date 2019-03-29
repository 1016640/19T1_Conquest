// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardManager.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "BoardManager"

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

	Player1SpawnHex = FIntVector(-1);
	Player2SpawnHex = FIntVector(-1);
}

#if WITH_EDITOR
void ABoardManager::CheckForErrors()
{
	Super::CheckForErrors();

	if (!HexGrid.bGridGenerated)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_NoBoardGenerated", "{ActorName} : Board Manager exists but no grid has been generated."), Arguments)))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorIsObselete));

		return;
	}
	
	if (GetPlayer1SpawnTile() == nullptr)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_NoP1Spawn", "{ActorName} : Board Manager has an invalid spawn point for Player 1."), Arguments)))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorIsObselete));
	}

	if (GetPlayer2SpawnTile() == nullptr)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_NoP1Spawn", "{ActorName} : Board Manager has an invalid spawn point for Player 2."), Arguments)))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorIsObselete));
	}
}
#endif

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

	// We can keep spawn points that still fit inside the new grid
	{
		if (!HexGrid.GridMap.Contains(Player1SpawnHex))
		{
			Player1SpawnHex = FIntVector(-1);
		}

		if (!HexGrid.GridMap.Contains(Player2SpawnHex))
		{
			Player2SpawnHex = FIntVector(-1);
		}
	}
}

void ABoardManager::SetPlayerSpawn(int32 Player, const FIntVector& TileHex)
{
	if (!ensure(Player >= 0 && Player <= 1))
	{
		UE_LOG(LogConquest, Warning, TEXT("Unable to set player spawn as player index is invalid"));
		return;
	}
	
	if (HexGrid.bGridGenerated)
	{
		// Make sure this tile actually belongs in our hex grid
		ATile* TileAtSpawn = HexGrid.GetTile(TileHex);
		if (TileAtSpawn)
		{
			if (Player == 0)
			{
				// Make sure this tile isn't already being used as a spawn point
				if (GetPlayer2SpawnTile() == TileAtSpawn)
				{
					Player2SpawnHex = FIntVector(-1);
				}

				Player1SpawnHex = TileHex;
			}
			else
			{
				// Make sure this tile isn't already being used as a spawn point
				if (GetPlayer1SpawnTile() == TileAtSpawn)
				{
					Player1SpawnHex = FIntVector(-1);
				}

				Player2SpawnHex = TileHex;
			}

			// Spawn points can't be null tiles
			TileAtSpawn->Modify();
			TileAtSpawn->bIsNullTile = false;
		}
	}
}
void ABoardManager::ResetPlayerSpawn(int32 Player)
{
	if (!ensure(Player >= 0 && Player <= 1))
	{
		UE_LOG(LogConquest, Warning, TEXT("Unable to reset player spawn as player index is invalid"));
		return;
	}

	if (Player == 0)
	{
		Player1SpawnHex = FIntVector(-1);
	}
	else
	{
		Player2SpawnHex = FIntVector(-1);
	}
}
#endif

#undef LOCTEXT_NAMESPACE