// Fill out your copyright notice in the Description page of Project Settings.

#include "HexGrid.h"
#include "Conquest.h"
#include "Tile.h"

const FHexGrid::FHex FHexGrid::DirectionTable[] =
{
	FHex(+1, -1, 0), FHex(+1, 0, -1), FHex(0, +1, -1),
	FHex(-1, +1, 0), FHex(-1, 0, +1), FHex(0, -1, +1)
};

void FHexGrid::GenerateGrid(int32 Rows, int32 Columns, const TFunction<ATile*(const FHex&, int32, int32)>& Predicate)
{
	GridMap.Empty();
	GridMap.Reserve(Rows * Columns);

	for (int32 c = 0; c < Columns; ++c)
	{
		int32 COffset = FMath::FloorToInt(c / 2);
		for (int32 r = -COffset; r < Rows - COffset; ++r)
		{
			FHex Hex = ConvertIndicesToHex(r, c);

			// Generate tile, we also inform of hex cell so
			// we can easily find specific cells later
			ATile* Tile = Predicate(Hex, r + COffset, c);
			if (!Tile)
			{
				UE_LOG(LogConquest, Warning, TEXT("Predicate for FHexGrid::GenerateGrid returned null"));
			}

			GridMap.Add(Hex, Tile);
		}
	}

	bGridGenerated = true;
}

void FHexGrid::ClearGrid()
{
	if (bGridGenerated)
	{
		// Destroy any existing tiles
		{
			TArray<ATile*> Tiles;
			GridMap.GenerateValueArray(Tiles);

			for (ATile* Tile : Tiles)
			{
				if (Tile)
				{
					Tile->Destroy();
				}
			}
		}

		GridMap.Empty();
		bGridGenerated = false;
	}
}
