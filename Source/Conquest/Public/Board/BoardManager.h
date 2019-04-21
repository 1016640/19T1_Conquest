// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Tile.h"
#include "Containers/HexGrid.h"
#include "BoardManager.generated.h"

class ATower;
class UMaterialInstanceConstant;
class UMaterialInterface;

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
	/** If board tiles should be drawn during PIE. This is client side only */
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

	/** If the given tile is a portal tile. Returns index of player if portal tile, else -1 */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	int32 IsPlayerPortalTile(const ATile* Tile) const;

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

	/** Get the tile at given location */
	FORCEINLINE ATile* GetTileAtLocation(const FVector& Location) const;

	/** Get all the tiles with a matching element type */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	TArray<ATile*> GetTilesWithMatchingElement(ECSKElementType Elements) const;

	/** Get all the tiles that are marked as null */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	TArray<ATile*> GetNullTiles() const;

	/** If a tower is allowed to be placed on given tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	bool CanPlaceTowerOnTile(const ATile* Tile) const;

private:

	/** Hex value for the first players portal (starting tile) */
	UPROPERTY()
	FIntVector Player1PortalHex;

	/** Hex value for the second players portal (starting tile) */
	UPROPERTY()
	FIntVector Player2PortalHex;

public:

	/** Traces the board to get a tile (can return null) */
	UFUNCTION(BlueprintCallable, Category = "Board")
	ATile* TraceBoard(const FVector& Origin, const FVector& End) const;

	/** Generates a path from the start tile to goal tile */
	UFUNCTION(BlueprintCallable, Category = "Board", meta = (AdvancedDisplay = 3))
	bool FindPath(const ATile* Start, const ATile* Goal, FBoardPath& OutPath, bool bAllowPartial = true, int32 MaxDistance = 100) const;

	/** Finds all the tiles that are within given amount of tiles from the origin */
	UFUNCTION(BlueprintCallable, Category = "Board")
	bool GetTilesWithinDistance(const ATile* Origin, int32 Distance, TArray<ATile*>& OutTiles, bool bIgnoreOccupiedTiles = true) const;

	/** Get all the tiles that are occupied and within given amount of tiles from the origin */
	UFUNCTION(BlueprintCallable, Category = "Board")
	bool GetOccupiedTilesWithinDistance(const ATile* Origin, int32 Distance, TArray<ATile*>& OutTiles, bool bIgnoreNullTiles = true, bool bIgnoreOrigin = true) const;

public:

	/** Attempts to place the board piece on given tile. This only runs on the server */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Board|Tiles")
	bool PlaceBoardPieceOnTile(AActor* BoardPiece, ATile* Tile) const;

	/** Clears the board piece set on given tile. This only runs on the server */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Board|Tiles")
	bool ClearBoardPieceOnTile(ATile* Tile) const;

public:

	/** Moves a board piece under the board based on it's boundaries */
	UFUNCTION(BlueprintCallable, Category = "Board", meta = (AdvancedDisplay=1))
	void MoveBoardPieceUnderBoard(AActor* BoardPiece, float Scale = 1.5f) const;

public:
	
	/** Forces a refresh of all tiles highlight materials */
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ForceTileHighlightRefresh();

	/** Get the highlight material associated with given element */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	UMaterialInstanceConstant* GetHighlightMaterialForElement(ECSKElementType ElementType) const;

	/** Get the highlight material for null tiles */
	FORCEINLINE UMaterialInstanceConstant* GetNullHighlightMaterial() const { return NullHighlightMaterial; }

	/** Get the highlight material for hovered tiles */
	FORCEINLINE UMaterialInstanceConstant* GetHoveredHighlightMaterial() const { return HoveredHighlightMaterial; }

	/** Get the highlight material for selectable tiles */
	FORCEINLINE UMaterialInstanceConstant* GetSelectableHighlightMaterial() const { return SelectableHighlightMaterial; }

	/** Get the highlight material for unselectable tiles */
	FORCEINLINE UMaterialInstanceConstant* GetUnselectableHighlightMaterial() const { return UnselectableHighlightMaterial; }

	/** Get the highlight material associated with player of ID */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	UMaterialInstanceDynamic* GetPlayerHighlightMaterial(int32 PlayerID) const;

public:

	/** Determines and sets the highlight material that given tile should use */
	void SetTilesHighlightMaterial(ATile* Tile) const;

	/** Sets the color for highlight material associated with player. This will create a new material if it doesn't exist */
	void SetHighlightColorForPlayer(int32 PlayerID, FLinearColor Color);

private:

	/** Sets the color for the highlight material associated with player ID. Will create it if it doesn't exist */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_SetHighlightColorForPlayer(int32 Player, FLinearColor Color);

protected:

	/** The highlight materials to use to highlight a tile based off its element type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board|Tiles")
	TMap<ECSKElementType, UMaterialInstanceConstant*> ElementHighlightMaterials;

	/** The highlight material to use to highlight a tile that is marked as null tile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board|Tiles")
	UMaterialInstanceConstant* NullHighlightMaterial;

	/** The highlight material to use for when a tile is hovered by the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board|Tiles")
	UMaterialInstanceConstant* HoveredHighlightMaterial;

	/** The highlight material to use for when a tile is marked as selectable (e.g. Can build a tower on) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board|Tiles")
	UMaterialInstanceConstant* SelectableHighlightMaterial;

	/** The highlight material to use for when a tile is marked as unselectable (e.g. Not a valid spell target) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Board|Tiles")
	UMaterialInstanceConstant* UnselectableHighlightMaterial;

	/** The highlight material to use to create highlight materials for specific players. This is used
	as a template to create the player highlight materials, and should contain a vector parameter named Color */
	UPROPERTY(EditAnywhere, Category = "Board|Tiles")
	UMaterialInterface* PlayerHighlightMaterialTemplate;

	/** The player highlight material associated with player 1 */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* Player1HighlightMaterial;

	/** The player highlight material associated with player 2 */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* Player2HighlightMaterial;
};

