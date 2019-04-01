// Fill out your copyright notice in the Description page of Project Settings.

#include "HexGrid.h"
#include "Conquest.h"
#include "Tile.h"

const FHexGrid::FHex FHexGrid::DirectionTable[] =
{
	FHex(+1, -1, 0), FHex(+1, 0, -1), FHex(0, +1, -1),
	FHex(-1, +1, 0), FHex(-1, 0, +1), FHex(0, -1, +1)
};

void FHexGrid::GenerateGrid(int32 Rows, int32 Columns, const TFunction<ATile*(const FHex&, int32, int32)>& Predicate, bool bClearGrid)
{
	if (bClearGrid)
	{
		ClearGrid();
	}
	else
	{
		// Clear only the tiles beyond new dimensions
		RemoveCellsFrom(Rows, Columns);
	}

	GridMap.Reserve(Rows * Columns);

	for (int32 c = 0; c < Columns; ++c)
	{
		int32 COffset = FMath::FloorToInt(c / 2);
		for (int32 r = -COffset; r < Rows - COffset; ++r)
		{
			FHex Hex = ConvertIndicesToHex(r, c);
			if (GridMap.Contains(Hex))
			{
				continue;
			}

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

	GridDimensions = FIntPoint(Rows, Columns);
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
		GridDimensions = FIntPoint::ZeroValue;
		bGridGenerated = false;
	}
}

void FHexGrid::RemoveCellsFrom(int32 Row, int32 Column)
{
	if (!bGridGenerated)
	{
		return;
	}

	if (Row < 1 || Column < 1)
	{
		ClearGrid();
		return;
	}

	int32 MaxRows = FMath::Max(Row, GridDimensions.X);
	int32 MaxCols = FMath::Max(Column, GridDimensions.Y);

	// Cycle through every cell and simply remove the cells
	// that lie beyond the specified limit
	for (int32 c = 0; c < MaxRows; ++c)
	{	
		for (int32 r = 0; r < MaxCols; ++r)
		{
			// Cell is out of range
			if (r >= Row || c >= Column)
			{
				int32 COffset = FMath::FloorToInt(c / 2);
				FHex Hex = ConvertIndicesToHex(r - COffset, c);
				
				ATile* Tile = nullptr;
				if (GridMap.RemoveAndCopyValue(Hex, Tile) && ensure(Tile != nullptr))
				{
					Tile->Destroy();
				}
			}
		}
	}

	GridMap.Shrink();
}

bool FHexGrid::GeneratePath(const FHex& Start, const FHex& Goal, bool bAllowPartial, FHexGridPathFindResultData& ResultData)
{
	// No grid
	if (!bGridGenerated)
	{
		ResultData.Set(EHexGridPathFindResult::NoGridGenerated);
		return false;
	}

	// Invalid hex (goal can still be treated as valid if allowing partial path)
	if (!GridMap.Contains(Start) || (!bAllowPartial && !GridMap.Contains(Goal)))
	{
		ResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}

	// TODO: Path find!

	return false;
}

bool FHexGrid::GeneratePath(ATile* Start, ATile* Goal, bool bAllowPartial, FHexGridPathFindResultData& ResultData)
{
	// Invalid tiles
	if (!Start || !Goal)
	{
		ResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}
	
	return GeneratePath(Start->GetGridHexValue(), Goal->GetGridHexValue(), bAllowPartial, ResultData);
}