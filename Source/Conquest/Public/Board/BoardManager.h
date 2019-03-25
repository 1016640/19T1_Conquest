// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "Containers/HexGrid.h"
#include "BoardManager.generated.h"

struct CONQUEST_API FBoardInitData
{
public:

	FORCEINLINE FBoardInitData(const FIntPoint& InDimensions, float InHexSize, const FVector& InOrigin, 
		const FRotator& InRotation, TSubclassOf<ATile> InTileTemplate = nullptr)
		: Dimensions(InDimensions)
		, HexSize(InHexSize)
		, Origin(InOrigin)
		, Rotation(InRotation)
		, TileTemplate(InTileTemplate)
	{
		check(IsValid());
	}

public:

	/** If the board data cached is valid */
	FORCEINLINE bool IsValid() const
	{
		bool bIsValid = true;

		// Dimensions should be 2 on at least both axes
		// so both players spawn on opposite ends
		bIsValid &= Dimensions.X >= 2;
		bIsValid &= Dimensions.Y >= 2;

		// Allow hex cells to have some space
		bIsValid &= HexSize >= 9.f;

		// Avoid NaN
		bIsValid &= !(Origin.ContainsNaN() || Rotation.ContainsNaN());

		return bIsValid;
	}

public:

	/** Dimensions of the board */
	FIntPoint Dimensions;

	/** The uniform size of each hex cell */
	float HexSize;

	/** The origin of the board */
	FVector Origin;

	/** The rotation of the board */
	FRotator Rotation;

	/** The tile class to use to generate the grid with (Can be null) */
	TSubclassOf<ATile> TileTemplate;
};

/**
 * Manages the board aspect of the game. Maintains each tile of the board
 * and can be used to query for paths or tiles around a specific tile
 */
UCLASS()
class CONQUEST_API ABoardManager : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ABoardManager();

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	void DrawDebugHexagon(const FVector& Position, float Size, float Time);

	UFUNCTION(Exec)
	void RebuildGrid();

public:

	/** Get the hex grid associated with the board */
	FORCEINLINE const FHexGrid& GetHexGrid() const { return HexGrid; }

	/** Get the size of a hex cell */
	FORCEINLINE float GetHexSize() const { return GridHexSize; }

public:

	UPROPERTY(EditAnywhere)
	uint8 bTestingToggle : 1;

	UPROPERTY(EditAnywhere)
	float HexSize;

	UPROPERTY(EditAnywhere)
	float DrawTime;

	UPROPERTY(EditAnywhere)
	int32 GridX;

	UPROPERTY(EditAnywhere)
	int32 GridY;

	UPROPERTY()
	FHexGrid HexGrid;

public:

	#if WITH_EDITOR
	/** Initializes the board using specified info */
	void InitBoard(const FBoardInitData& InitData);
	#endif

protected:

	/** The dimensions of the board */
	UPROPERTY(VisibleAnywhere, Category = "Board", meta = (DisplayName="Board Dimensions"))
	FIntPoint GridDimensions;

	/** The size of each cell of the baord */
	UPROPERTY(VisibleAnywhere, Category = "Board", meta = (DisplayName="Board Dimensions"))
	float GridHexSize;
};

