// Fill out your copyright notice in the Description page of Project Settings.

#include "HexGrid.h"
#include "Conquest.h"
#include "Tile.h"

DECLARE_CYCLE_STAT(TEXT("HexGrid FindPath"), STAT_HexGridFindPath, STATGROUP_Conquest);

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

bool FHexGrid::GeneratePath(const FHex& Start, const FHex& Goal, FHexGridPathFindResultData& OutResultData, bool bAllowPartial, int32 MaxDistance) const
{
	// No grid
	if (!bGridGenerated)
	{
		OutResultData.Set(EHexGridPathFindResult::NoGridGenerated);
		return false;
	}

	// Invalid distance
	if (MaxDistance <= 0)
	{
		OutResultData.Set(EHexGridPathFindResult::InvalidDistance);
		return false;
	}

	// Invalid hex (goal can still be treated as valid if allowing partial path)
	if (!GridMap.Contains(Start) || (!bAllowPartial && !GridMap.Contains(Goal)))
	{
		OutResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}

	// Already at goal
	if (Start == Goal)
	{
		OutResultData.Set(EHexGridPathFindResult::AlreadyAtGoal);
		return true;
	}

	// Start is blocked
	const ATile* StartTile = GetTile(Start);
	if (!StartTile || StartTile->bIsNullTile)
	{
		OutResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}

	// Goal is blocked (goal can still be treated as valid if allowing partial path)
	const ATile* GoalTile = GetTile(Goal);
	if (!GoalTile || (!bAllowPartial && GoalTile->bIsNullTile))
	{
		OutResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}

	return FindPath(Start, Goal, OutResultData, bAllowPartial, MaxDistance);
}

bool FHexGrid::GeneratePath(const ATile* Start, const ATile* Goal, FHexGridPathFindResultData& OutResultData, bool bAllowPartial, int32 MaxDistance) const
{
	// Invalid tiles
	if (!Start || !Goal)
	{
		OutResultData.Set(EHexGridPathFindResult::InvalidTargets);
		return false;
	}
	
	return GeneratePath(Start->GetGridHexValue(), Goal->GetGridHexValue(), OutResultData, bAllowPartial, MaxDistance);
}

// TODO: Needs to be fixed (as in partial path returning the tile that was closest to the goal, it doesn't right now)
bool FHexGrid::FindPath(const FHex& Start, const FHex& Goal, FHexGridPathFindResultData& OutResultData, bool bAllowPartial, int32 MaxDistance) const
{
	struct FPathSegment
	{
		FPathSegment()
			: Hex(0)
			, Cost(FLT_MAX)
			, Distance(0)
		{

		}

		FPathSegment(const FHex& InHex, float InCost, int32 InDistance)
			: Hex(InHex)
			, Cost(InCost)
			, Distance(InDistance)
		{

		}

		// Hex for this current segment
		FHex Hex;

		// Cost for reaching this segment
		float Cost;

		// Amount of tiles since origin
		int32 Distance;
	};

	// Predicate for sorting queue
	struct FPathPredicate
	{
		bool operator() (const FPathSegment& lhs, const FPathSegment rhs) const
		{
			return lhs.Cost > rhs.Cost;
		}
	};

	SCOPE_CYCLE_COUNTER(STAT_HexGridFindPath);
	
	const ATile* GoalTile = GetTile(Goal);
	check(GoalTile);

	bool bGoalFound = false;
	FHex LastHex = FHex(-1);

	// Hex indices we have already visited
	TSet<FHex> Visited;

	// Edges between the path being constructed
	TMap<FHex, FHex> PathEdges;
	PathEdges.Add(Start, Start);

	// Queue with cheapest hex tiles placed in the front
	TArray<FPathSegment> Queue;
	Queue.HeapPush(FPathSegment(Start, 0.f, 0), FPathPredicate());

	while (true)
	{
		// Are we unable to continue?
		if (Queue.Num() == 0)
		{
			break;
		}

		// Remove this hex from the queue
		FPathSegment Segment;
		Queue.HeapPop(Segment, FPathPredicate());
		LastHex = Segment.Hex;
		
		// Have we reached our target?
		if (Segment.Hex == Goal)
		{
			bGoalFound = true;
			break;
		}

		// Marked as visit, so we don't return here
		Visited.Add(Segment.Hex);
	
		int32 BestNeighborIndex = -1;
		FHex BestNeighborHex = FHex(-1);
		float BestNeighborCost = FLT_MAX;

		// If goal was found but is null, we should forcefully exit
		bool bForceExit = false;
		
		// Cycle through all this segments neighbors to determine which path to prioritize
		TArray<FHex> Neighbors = GetNeighbors(Segment.Hex);
		for (int32 i = 0; i < Neighbors.Num(); ++i)
		{
			const FHex& Neighbor = Neighbors[i];
			const ATile* NeighborTile = GetTile(Neighbor);

			// Ignore visited tiles as they probaly are in the queue
			if (!Visited.Contains(Neighbor))
			{
				// Don't bother processing this tile since it's occupied
				if (NeighborTile->IsTileOccupied())
				{
					// If finding partial paths, there is a chance of the goal tile being null
					if (NeighborTile == GoalTile)
					{
						// Stop the search, we know we can't reach the goal
						bForceExit = true;
						break;
					}
					else
					{
						// There is still a chance of reaching the goal
						continue;
					}
				}

				float NewCost = Segment.Cost + PathHeuristic(NeighborTile, GoalTile);
				if (NewCost < BestNeighborCost)
				{
					BestNeighborIndex = i;
					BestNeighborHex = Neighbor;
					BestNeighborCost = NewCost;
				}
			}
		}

		if (bForceExit)
		{
			break;
		}

		// Can we still continue down this path?
		if (BestNeighborIndex != -1)
		{
			int32 NewDistance = Segment.Distance + 1;
			if (NewDistance <= MaxDistance)
			{
				// Establish new path edge (increment distance)
				Queue.HeapPush(FPathSegment(BestNeighborHex, BestNeighborCost, Segment.Distance + 1), FPathPredicate());
				PathEdges[Segment.Hex] = BestNeighborHex;

				PathEdges.Add(BestNeighborHex, BestNeighborHex);
			}
		}
	}

	// Did we exit by reaching the goal?
	if (bGoalFound)
	{
		TArray<ATile*> Tiles;
		ConvertEdgesToPath(Start, Goal, PathEdges, Tiles);

		OutResultData.Set(EHexGridPathFindResult::Success, Tiles);
	}
	// We exited without reaching the goal but we can still use a partial path
	else if (bAllowPartial)
	{
		TArray<ATile*> Tiles;
		ConvertEdgesToPath(Start, LastHex, PathEdges, Tiles);

		OutResultData.Set(EHexGridPathFindResult::Partial, Tiles);

		// We still managed to find something
		bGoalFound = true;
	}
	else
	{
		OutResultData.Set(EHexGridPathFindResult::Failure);
	}

	return bGoalFound;
}

void FHexGrid::ConvertEdgesToPath(const FHex& Start, const FHex& Goal, const TMap<FHex, FHex>& Edges, TArray<ATile*>& OutPath) const
{
	OutPath.Empty();

	// Cycle through the path edges that were generated
	// to get the path of tiles in the correct order
	FHex Current = Start;
	while (Current != Goal)
	{
		OutPath.Add(GetTile(Current));

		// Move onto next segment
		Current = Edges[Current];
	}

	// Add goal tile as well
	OutPath.Add(GetTile(Goal));
}

float FHexGrid::PathHeuristic(const ATile* T1, const ATile* T2)
{
	if (T1 && T2)
	{
		return T1->GetSquaredDistanceTo(T2);
	}

	return MAX_FLT;
}
