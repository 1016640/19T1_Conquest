// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HexGrid.generated.h"

class ATile;

/**
 * A grid genereted using hexagons. This grid uses cube coordinates
 * and is specifically designed for use in CSK. I highly recommend
 * and thank RedBlob Games for their article on Hex Grids
 * https://www.redblobgames.com/grids/hexagons/
 */
USTRUCT()
struct CONQUEST_API FHexGrid
{
	GENERATED_BODY()

public:

	using FHex = FIntVector;
	using FFracHex = FVector;

	static constexpr float StartAngle()
	{
		return 0.5f;
	}

public:

	FHexGrid()
		: bGridGenerated(false)
	{
		
	}

private:

	/** Lookup table for travelling */
	static const FHex DirectionTable[6];

	FORCEINLINE static bool IsValidHex(const FHex& Hex)
	{
		return (Hex.X + Hex.Y + Hex.Z) == 0;
	}

	FORCEINLINE static const FHex& HexDirection(int32 Index)
	{
		check(Index >= 0 && Index <= 5);
		return DirectionTable[Index];
	}

	FORCEINLINE static int32 HexLength(const FHex& Hex)
	{
		return Hex.GetMax();
	}

	FORCEINLINE static int32 HexDisplacement(const FHex& H1, const FHex& H2)
	{
		return HexLength(H1 - H2);
	}

	FORCEINLINE static FHex HexRound(const FFracHex& FracHex)
	{
		float X = FMath::RoundToFloat(FracHex.X);
		float Y = FMath::RoundToFloat(FracHex.Y);
		float Z = FMath::RoundToFloat(FracHex.Z);

		float DeltaX = FMath::Abs(X - FracHex.X);
		float DeltaY = FMath::Abs(Y - FracHex.Y);
		float DeltaZ = FMath::Abs(Z - FracHex.Z);

		// Need to assure that hex length will equal zero
		if (DeltaX > DeltaY && DeltaX > DeltaZ)
		{
			X = -Y - Z;
		}
		else if (DeltaY > DeltaZ)
		{
			Y = -X - Z;
		}
		else
		{
			Z = -X - Y;
		}

		FHex HexValue(static_cast<int32>(X), static_cast<int32>(Y), static_cast<int32>(Z));
		check(IsValidHex(HexValue));

		return HexValue;
	}

public:

	/** Get row and column indices as a hex cell */
	FORCEINLINE static FHex ConvertIndicesToHex(int32 Row, int32 Column)
	{
		FHex Hex(Row, Column, -Row - Column);
		check(IsValidHex(Hex));

		return Hex;
	}

	/** Converts a hex cell into a world position based off an origin and cell size.
	The cell is only applied onto the XY plane, with Z being the same as Origin.Z */
	static FVector ConvertHexToWorld(const FHex& Hex, const FVector& Origin, const FVector& Size)
	{
		const float f0 = FMath::Sqrt(3.f);
		const float f1 = f0 / 2.f;
		const float f2 = 0.f;
		const float f3 = 3.f / 2.f;

		float X = (f0 * static_cast<float>(Hex.X) + f1 * static_cast<float>(Hex.Y)) * Size.X;
		float Y = (f2 * static_cast<float>(Hex.X) + f3 * static_cast<float>(Hex.Y)) * Size.Y;

		return FVector(Origin.X + X, Origin.Y + Y, Origin.Z);
	}

	/** Converts a world position to a hex cell. Z axis does not affect calculation */
	FORCEINLINE static FHex ConvertWorldToHex(const FVector& Position, const FVector& Origin, const FVector& Size)
	{
		const float b0 = FMath::Sqrt(3.f) / 3.f;
		const float b1 = -1.f / 3.f;
		const float b2 = 0.f;
		const float b3 = 2.f / 3.f;

		float PointX = (Position.X - Origin.X) / Size.X;
		float PointY = (Position.Y - Origin.Y) / Size.Y;

		float X = b0 * PointX + b1 * PointY;
		float Y = b2 * PointX + b3 * PointY;

		return HexRound(FFracHex(X, Y, -X - Y));
	}

public:

	/** Generates a rectangular shaped map with given rows and columns.
	Takes in a predicate which is used to initialize each cell, predicate
	should expect the hex index, and the cells row and column index */
	void GenerateGrid(int32 Rows, int32 Columns, const TFunction<ATile*(const FHex&, int32, int32)>& Predicate);

	/** Clears the grid, will destroy all tiles that have been spawned */
	void ClearGrid();

public:

	/** Get all tiles in the grid */
	FORCEINLINE TArray<ATile*> GetAllTiles() const
	{
		TArray<ATile*> Tiles;
		GridMap.GenerateValueArray(Tiles);

		return Tiles;
	}

public:

	/** Map containing all tiles in the map */
	UPROPERTY()
	TMap<FIntVector, ATile*> GridMap;

	/** If the grid has been generated */
	UPROPERTY()
	uint8 bGridGenerated : 1;
};