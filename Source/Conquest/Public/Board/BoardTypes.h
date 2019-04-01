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

public:

	/** The tiles to follow, in order from first to last */
	UPROPERTY(BlueprintReadOnly)
	TArray<ATile*> Path;
};