// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardManager.h"
#include "Tile.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ABoardManager::ABoardManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

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

	bTestingToggle = false;
	HexSize = 200.f;
	DrawTime = 5.f;

	GridX = 10;
	GridY = 10;
}

void ABoardManager::OnConstruction(const FTransform& Transform)
{
	//if (!bTestingToggle)
	//	return;

	//FVector GridOrigin = GetActorLocation();
	//FVector GridSize(HexSize, HexSize, 0.f);

	//auto GeneratePredicate = [this, GridOrigin, GridSize](const FHexGrid::FHex& Hex, int32 Row, int32 Column)->ATile*
	//{
	//	UWorld* World = this->GetWorld();
	//	if (World)
	//	{
	//		FVector TileLocation = FHexGrid::ConvertHexToWorld(Hex, GridOrigin, GridSize);
	//		ATile* Tile = nullptr; //World->SpawnActor<ATile>(ATile::StaticClass(), TileLocation, FRotator::ZeroRotator);
	//		if (true)//Tile)
	//		{
	//			//Tile->Hex = Hex;
	//			//Tile->Row = Row;
	//			//Tile->Column = Column;

	//			this->DrawDebugHexagon(TileLocation, HexSize, DrawTime);
	//			this->DrawDebugHexagon(TileLocation, HexSize * .75f, DrawTime);
	//			// Potential init
	//		}

	//		return Tile;
	//	}

	//	return nullptr;
	//};

	//// for now
	//TArray<ATile*> ts;
	//HexGrid.GridMap.GenerateValueArray(ts);

	//for (auto t : ts)
	//{
	//	if (t)
	//		t->Destroy();
	//}

	//HexGrid.GenerateGrid(GridX, GridY, GeneratePredicate);

	////FVector2D GridSize((float)HexSize, (float)HexSize);
	////auto hex_corner_offset = [this, GridSize](float start_angle, int32 corner)->FVector
	////{
	////	float angle = 2.f * PI * (start_angle + (float)corner) / 6;
	////	return FVector(GridSize.X * FMath::Cos(angle), GridSize.Y * sin(angle), this->GetActorLocation().Z);
	////};
	//
	///*auto hex_to_world = [this, GridSize](FIntVector hex)->FVector
	//{
	//	float X = (FMath::Sqrt(3.f) * hex.X + (FMath::Sqrt(3.f) / 2.f) * hex.Y) * GridSize.X;
	//	float Y = (0.f * hex.X + (3.f / 2.f) * hex.Y) * GridSize.Y;

	//	return FVector(this->GetActorLocation().X + X, this->GetActorLocation().Y + Y, this->GetActorLocation().Z);
	//};

	//for (int32 Y = 0; Y < GridY; ++Y)
	//{
	//	int32 YOffset = FMath::FloorToInt(Y / 2);
	//	for (int32 X = -YOffset; X < GridX - YOffset; ++X)
	//	{
	//			bool bSplitX = X % 2 == 1;
	//			bool bSplitY = Y % 2 == 1;

	//			FVector Pos = GetActorLocation();
	//			Pos.X += (FMath::Sqrt(3) * HexSize) * (float)X;
	//			Pos.Y += (2 * HexSize) * (float)Y;

	//			if (bSplitY)
	//			{
	//				Pos.X += (FMath::Sqrt(3) * HexSize) / 2.f;
	//			}

	//			if (bSplitY)
	//			{
	//				Pos.Y -= (HexSize) * 0.5f;
	//			}

	//			if (Y > 1)
	//			{
	//				Pos.Y -= (HexSize) * 0.5f;
	//			}

	//			Pos = hex_to_world(FIntVector(X, Y, 0));
	//			DrawDebugHexagon(Pos, HexSize, DrawTime);
	//	}
	//}*/
}

void ABoardManager::DrawDebugHexagon(const FVector& Position, float Size, float Time)
{
	auto GetPoint = [this](const FVector& Pos, float Size, int32 Index)->FVector
	{
		float Angle = 60.f * (float)Index - 30.f;
		float Radians = FMath::DegreesToRadians(Angle);

		return FVector(Pos.X + Size * cos(Radians), Pos.Y + Size * sin(Radians), this->GetActorLocation().Z);
	};

	FVector CurrentPoint = GetPoint(Position, Size, 0);
	for (int32 I = 0; I <= 5; ++I)
	{
		FVector NextPoint = GetPoint(Position, Size, (I + 1) % 6);
		DrawDebugLine(GetWorld(), CurrentPoint, NextPoint, FColor::Green, false, Time, 0, 5.f);

		CurrentPoint = NextPoint;
	}

	DrawDebugSphere(GetWorld(), Position, Size / 2.f, 4, FColor::Red, false, Time, 0, 5.f);
}

void ABoardManager::RebuildGrid()
{
	OnConstruction(GetActorTransform());
}

#if WITH_EDITOR
void ABoardManager::InitBoard(const FBoardInitData& InitData)
{
	check(InitData.IsValid());

	// Clear all existing tiles // TODO: Only remove expired tiles (and avoiding re-creating all tiles)
	HexGrid.ClearGrid();

	GridDimensions = InitData.Dimensions;
	GridHexSize = InitData.HexSize;
	SetActorLocationAndRotation(InitData.Origin, InitData.Rotation);

	// If no tile has been specified, simply use the default one
	TSubclassOf<ATile> TileTemplate = InitData.TileTemplate != nullptr ? InitData.TileTemplate : ATile::StaticClass();

	// The size of each cell
	FVector GridSize(GridHexSize, GridHexSize, 0.f);

	int32 CellID = 0;

	// Predicate that will spawn a tile specified by the hex grid during its generation
	auto TilePredicate = [this, GridSize, TileTemplate, &CellID](const FHexGrid::FHex& Hex, int32 Row, int32 Column)->ATile*
	{
		UWorld* World = this->GetWorld();
		if (World)
		{
			FVector TileLocation = FHexGrid::ConvertHexToWorld(Hex, this->GetActorLocation(), GridSize);
			FString TileID = FString("BoardTile") + FString::FromInt(CellID);

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			//SpawnParams.Name = FName(*TileID); // This is causing weird issues when rebuilding (assuming this is due to these actors potentially existing at this point)
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			// Init tile
			ATile* Tile = World->SpawnActor<ATile>(TileTemplate, TileLocation, FRotator::ZeroRotator, SpawnParams);
			if (Tile)
			{
				// Easy identifier for in editor work (TODO: Have hex be printed instead of ID)
				Tile->SetActorLabel(TileID);

				Tile->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				Tile->Hex = Hex;
				// TODO: prob more init
			}

			return Tile;
		}

		return nullptr;
	};

	// Generate the grid
	HexGrid.GenerateGrid(GridDimensions.X, GridDimensions.Y, TilePredicate);
}
#endif
