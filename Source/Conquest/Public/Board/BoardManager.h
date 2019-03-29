// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Tile.h"
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

	/** Get validated tile template (should be used over accessing it directly) */
	FORCEINLINE TSubclassOf<ATile> GetTileTemplate() const
	{
		return TileTemplate != nullptr ? TileTemplate : ATile::StaticClass();
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
UCLASS(notplaceable)
class CONQUEST_API ABoardManager : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ABoardManager();

protected:

	#if WITH_EDITOR
	// Begin AActor Interface
	virtual void CheckForErrors() override;
	// End AActor Interface
	#endif

public:

	/** Destroys this board (including tiles) */
	void DestroyBoard();

public:

	/** Get the hex grid associated with the board */
	FORCEINLINE const FHexGrid& GetHexGrid() const { return HexGrid; }

	/** Get the dimensions of the grid */
	FORCEINLINE const FIntPoint& GetGridDimensions() const { return GridDimensions; }

	/** Get the size of a hex cell */
	FORCEINLINE float GetGridHexSize() const { return GridHexSize; }

	#if WITH_EDITOR
	/** Get last tile template used to generate the grid with */
	FORCEINLINE TSubclassOf<ATile> GetGridTileTemplate() const { return GridTileTemplate; }
	#endif
	

public:

	#if WITH_EDITOR
	/** Initializes the board using specified info */
	void InitBoard(const FBoardInitData& InitData);

	/** Set the spawn tile for the specified player */
	void SetPlayerSpawn(int32 Player, const FIntVector& TileHex);

	/** Resets the player spawn tile to invalid */
	void ResetPlayerSpawn(int32 Player);
	#endif

protected:

	/** The hex grid containing all tiles of the board */
	UPROPERTY()
	FHexGrid HexGrid;

	/** The dimensions of the board */
	UPROPERTY(VisibleAnywhere, Category = "Board", meta = (DisplayName="Board Dimensions"))
	FIntPoint GridDimensions;

	/** The size of each cell of the baord */
	UPROPERTY(VisibleAnywhere, Category = "Board", meta = (DisplayName="Board Dimensions"))
	float GridHexSize;

	#if WITH_EDITORONLY_DATA
	/** Uses along side board editor mode so board settings can update appropriately */
	UPROPERTY(VisibleAnywhere, Category = "Board", meta = (DisplayName = "Board Tile Type"))
	TSubclassOf<ATile> GridTileTemplate;
	#endif

public:

	/** Get the tile marked as being player 1 spawn (can return null) */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ATile* GetPlayer1SpawnTile() const { return GetTileAt(Player1SpawnHex); }

	/** Get the tile marked as being player 2 spawn (can return null) */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ATile* GetPlayer2SpawnTile() const { return GetTileAt(Player2SpawnHex); }

	/** Get the spawn tile for the specified player */
	FORCEINLINE ATile* GetPlayerSpawnTile(int32 Player) const
	{
		if (ensureMsgf(Player >= 0 && Player <= 1, TEXT("Player index must be 0 for player 1 or 1 for player 2")))
		{
			if (Player == 0)
			{
				return GetPlayer1SpawnTile();
			}
			else
			{
				return GetPlayer2SpawnTile();
			}
		}

		return nullptr;
	}

	/** Get the tile at given hex value */
	FORCEINLINE ATile* GetTileAt(const FIntVector& Hex) const { return HexGrid.GetTile(Hex); }

private:

	/** Hex value for the first players starting tile */
	UPROPERTY()
	FIntVector Player1SpawnHex;

	/** Hex value for the second players starting tile */
	UPROPERTY()
	FIntVector Player2SpawnHex;
};

