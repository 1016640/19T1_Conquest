// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardManager.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/BillboardComponent.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "MapErrors.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "BoardManager"

ABoardManager::ABoardManager()
{
	// We only need to tick in editor
	#if WITH_EDITORONLY_DATA
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	#endif

	bReplicates = false;
	bNetLoadOnClient = true;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->SetMobility(EComponentMobility::Static);

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
	Player1PortalHex = FIntVector(-1);
	Player2PortalHex = FIntVector(-1);

	#if WITH_EDITORONLY_DATA
	GridTileTemplate = nullptr;
	bDrawDebugBoard = true;
	#endif
}

void ABoardManager::BeginPlay()
{
	Super::BeginPlay();

	#if WITH_EDITORONLY_DATA
	SetActorTickEnabled(bDrawDebugBoard);
	#endif
}

void ABoardManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITORONLY_DATA
	if (bDrawDebugBoard && HexGrid.bGridGenerated)
	{
		// Draws a hexagon based on a tile (not a static lambda since we might be PIE testing with more than 1 client)
		auto DrawHexagon = [this](ATile* Tile, const FColor& Color, uint8 Depth)->void
		{
			UWorld* World = this->GetWorld();
			FVector Position = Tile->GetActorLocation();

			FVector CurrentVertex = FHexGrid::ConvertHexVertexIndexToWorld(Position, this->GridHexSize, 0);
			for (int32 i = 0; i <= 5; ++i)
			{
				FVector NextVertex = FHexGrid::ConvertHexVertexIndexToWorld(Position, this->GridHexSize, (i + 1) % 6);
				DrawDebugLine(World, CurrentVertex, NextVertex, Color, false, -1.f, Depth, 5.f);

				CurrentVertex = NextVertex;
			}
		};

		TArray<ATile*> Tiles = HexGrid.GetAllTiles();
		for (ATile* Tile : Tiles)
		{
			if (Tile)
			{
				if (Tile->bHighlightTile)
				{
					DrawHexagon(Tile, FColor::Magenta, 2);
				}
				else if (Tile->bIsNullTile)
				{
					DrawHexagon(Tile, FColor::Black, 1);
				}
				else
				{
					DrawHexagon(Tile, FColor::Emerald, 0);
				}
			}
		}
	}
	#endif
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
	
	if (GetPlayer1PortalTile() == nullptr)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_NoP1Spawn", "{ActorName} : Board Manager has an invalid spawn point for Player 1."), Arguments)))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorIsObselete));
	}

	if (GetPlayer2PortalTile() == nullptr)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("ActorName"), FText::FromString(GetPathName()));
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_NoP1Spawn", "{ActorName} : Board Manager has an invalid spawn point for Player 2."), Arguments)))
			->AddToken(FMapErrorToken::Create(FMapErrors::ActorIsObselete));
	}
}

void ABoardManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABoardManager, bDrawDebugBoard))
	{
		UWorld* World = GetWorld();
		if (World && World->IsPlayInEditor())
		{
			SetActorTickEnabled(bDrawDebugBoard);
		}
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

	// We can keep portals that still fit inside the new grid
	{
		if (!HexGrid.GridMap.Contains(Player1PortalHex))
		{
			Player1PortalHex = FIntVector(-1);
		}

		if (!HexGrid.GridMap.Contains(Player2PortalHex))
		{
			Player2PortalHex = FIntVector(-1);
		}
	}
}

void ABoardManager::SetPlayerPortal(int32 Player, const FIntVector& TileHex)
{
	if (!ensure(Player >= 0 && Player <= 1))
	{
		UE_LOG(LogConquest, Warning, TEXT("Unable to set player portal as player index is invalid"));
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
				// Make sure this tile isn't already being used as a portal
				if (GetPlayer2PortalTile() == TileAtSpawn)
				{
					Player2PortalHex = FIntVector(-1);
				}

				Player1PortalHex = TileHex;
			}
			else
			{
				// Make sure this tile isn't already being used as a portal
				if (GetPlayer1PortalTile() == TileAtSpawn)
				{
					Player1PortalHex = FIntVector(-1);
				}

				Player2PortalHex = TileHex;
			}

			// Spawn points can't be null tiles
			TileAtSpawn->Modify();
			TileAtSpawn->bIsNullTile = false;
		}
	}
}
void ABoardManager::ResetPlayerPortal(int32 Player)
{
	if (!ensure(Player >= 0 && Player <= 1))
	{
		UE_LOG(LogConquest, Warning, TEXT("Unable to reset player portal as player index is invalid"));
		return;
	}

	if (Player == 0)
	{
		Player1PortalHex = FIntVector(-1);
	}
	else
	{
		Player2PortalHex = FIntVector(-1);
	}
}
#endif

ATile* ABoardManager::TraceBoard(const FVector& Origin, const FVector& End) const
{
	ATile* Tile = nullptr;
	if (HexGrid.bGridGenerated)
	{
		FVector PlaneOrigin = GetActorLocation();
		FVector PlaneNormal = GetActorUpVector();

		FVector HitPoint = FMath::LinePlaneIntersection(Origin, End, FPlane(PlaneOrigin, PlaneNormal));
		Tile = HexGrid.GetTile(FHexGrid::ConvertWorldToHex(HitPoint, PlaneOrigin, FVector(GridHexSize)));
	}

	return Tile;
}

bool ABoardManager::FindPath(const ATile* Start, const ATile* Goal, FBoardPath& OutPath, bool bAllowPartial, int32 MaxDistance) const
{
	bool bSuccess = false;
	FHexGridPathFindResultData ResultData;
	if (HexGrid.GeneratePath(Start, Goal, ResultData, bAllowPartial, MaxDistance))
	{
		OutPath.Path = ResultData.Path;
		bSuccess = true;
	}

	return bSuccess;
}

#undef LOCTEXT_NAMESPACE