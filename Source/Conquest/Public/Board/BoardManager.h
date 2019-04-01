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

public:

	#if WITH_EDITORONLY_DATA
	/** If board tiles should be drawn during PIE */
	UPROPERTY(EditInstanceOnly, Category = Debug)
	uint32 bDrawDebugBoard : 1;
	#endif

public:

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:
	
	// Begin AActor Interface
	#if WITH_EDITOR
	virtual void CheckForErrors() override;
	#endif
	// End AActor Interface

	// Begin UObject Interface
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface

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

	/** Set the portal tile for the specified player */
	void SetPlayerPortal(int32 Player, const FIntVector& TileHex);

	/** Clears portal tile used for specified player */
	void ResetPlayerPortal(int32 Player);
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

	/** Traces the board to get a tile (can return null) */
	UFUNCTION(BlueprintCallable, Category = "Board")
	ATile* TraceBoard(const FVector& Origin, const FVector& End) const;

public:

	/** Get the tile marked as being player 1 portal (can return null) */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ATile* GetPlayer1PortalTile() const { return GetTileAt(Player1PortalHex); }

	/** Get the tile marked as being player 2 portal (can return null) */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ATile* GetPlayer2PortalTile() const { return GetTileAt(Player2PortalHex); }

	/** Get the portal tile for the specified player */
	FORCEINLINE ATile* GetPlayerPortalTile(int32 Player) const
	{
		if (ensureMsgf(Player >= 0 && Player <= 1, TEXT("Player index must be 0 for player 1 or 1 for player 2")))
		{
			if (Player == 0)
			{
				return GetPlayer1PortalTile();
			}
			else
			{
				return GetPlayer2PortalTile();
			}
		}

		return nullptr;
	}

	/** Get the tile at given hex value */
	FORCEINLINE ATile* GetTileAt(const FIntVector& Hex) const { return HexGrid.GetTile(Hex); }

private:

	/** Hex value for the first players portal (starting tile) */
	UPROPERTY()
	FIntVector Player1PortalHex;

	/** Hex value for the second players portal (starting tile) */
	UPROPERTY()
	FIntVector Player2PortalHex;
};

