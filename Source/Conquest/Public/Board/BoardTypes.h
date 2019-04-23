// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "BoardTypes.generated.h"

class ATile;

/** All elements present in CSK (bitset since some spells have 2 types) */
UENUM(BlueprintType, meta = (DisplayName = "ElementType", Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECSKElementType : uint8
{
	Fire	= 1,
	Water	= 2,
	Earth	= 4,
	Air		= 8,

	All		= 15	UMETA(Hidden="true"),
	None	= 0		UMETA(Hidden="true")
};

ENUM_CLASS_FLAGS(ECSKElementType);

/** Tracks if a tile has been marked as selectable or unselectable. This is used to determine
which highlight material to use when a tile is marked as either selectable or unselectable while
the player is currently hovering over the tile */
UENUM(BlueprintType)
enum class ETileSelectionState : uint8
{
	/** Tile is not selectable */
	NotSelectable,

	/** Tile is marked as selectable, but does not have priority over hover */
	Selectable,

	/** Tile is marked as selectable and has priority over hover */
	SelectablePriority,

	/** Tile is marked as unselectable, but does not priority over hover */
	Unselectable,

	/** Tile is marked as unselectable and has priority over hover */
	UnselectablePriority
};

/** A path for traversing the board */
USTRUCT(BlueprintType)
struct CONQUEST_API FBoardPath
{
	GENERATED_BODY()

public:

	FBoardPath()
	{

	}

	FBoardPath(TArray<ATile*>&& InPath)
		: Path(InPath)
	{

	}

public:

	/** Get if this path is valid */
	FORCEINLINE bool IsValid() const
	{
		return Path.Num() > 0;
	}

	/** Resets this path to invalid state */
	FORCEINLINE void Reset()
	{
		Path.Empty();
	}

	/** Get the number of tiles in this path */
	FORCEINLINE int32 Num() const
	{
		return Path.Num();
	}

	/** Get the last tile in the path */
	FORCEINLINE ATile* Goal() const
	{
		if (IsValid())
		{
			return Path.Last();
		}

		return nullptr;
	}

public:

	FORCEINLINE ATile* operator [] (int32 Index)
	{
		return Path[Index];
	}

	FORCEINLINE ATile* operator [] (int32 Index) const
	{
		return Path[Index];
	}

public:

	/** The tiles to follow, in order from first to last */
	UPROPERTY(BlueprintReadOnly)
	TArray<ATile*> Path;
};