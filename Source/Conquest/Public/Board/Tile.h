// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "GameFramework/Actor.h"
#include "Tile.generated.h"

class ACSKPlayerController;
class ACSKPlayerState;
class IBoardPieceInterface;
class UHealthComponent;
class UStaticMeshComponent;

/**
 * Actor for managing an individual tile on the board. Will track
 * any tower (including castles) that have been built upon it
 */
UCLASS()
class CONQUEST_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	
	ATile();

public:

	#if WITH_EDITORONLY_DATA
	/** If this tile should be highlighted during PIE */
	UPROPERTY(EditInstanceOnly, Category = Debug)
	uint32 bHighlightTile : 1;
	#endif

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface

public:

	/** Get mesh component */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

private:

	/** Animated skeletal mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

public:

	/** Set this tiles hex value on the board */
	FORCEINLINE void SetGridHexValue(const FIntVector& Hex) { GridHexIndex = Hex; }

	/** Get this tiles hex value on the board */
	FORCEINLINE const FIntVector& GetGridHexValue() const { return GridHexIndex; }

public:

	/** The element type of this tile */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Tile)
	ECSKElementType TileType;

	/** If this tile is a null tyle (no go zone) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	uint32 bIsNullTile : 1;

protected:

	/** This tiles hex value in the board managers grid. Is
	used to determine this tiles position amongst all others */
	UPROPERTY(VisibleAnywhere)
	FIntVector GridHexIndex;

public:

	/** Informs this tile that given player is now hovering over it */
	void StartHoveringTile(ACSKPlayerController* Controller);

	/** Informs this tile thay given player is no longer hovering over it */
	void EndHoveringTile(ACSKPlayerController* Controller);

	/** Sets this tile selectable state for visual purposes */
	void SetSelectionState(ETileSelectionState State);

public:

	/** Get if this tile is currently being hovered by the player */
	UFUNCTION(BlueprintPure, Category = Tile)
	bool IsHovered() const { return HoveringPlayer.IsValid(); }

	/** Get this tiles selection state. Do not use this for game logic but for aesthetic logic */
	UFUNCTION(BlueprintPure, Category = Tile)
	ETileSelectionState GetSelectionState() const { return SelectionState; }

protected:

	/** Event for when the player has started to hover over this tile. This event will only ever be called on the client */
	UFUNCTION(BlueprintImplementableEvent, Category = Tile, meta = (DisplayName = "On Hover Start"))
	void BP_OnHoverStart(ACSKPlayerController* Controller);

	/** Event for when the player is no longer hovering over this tile. This event will only ever be called on the client */
	UFUNCTION(BlueprintImplementableEvent, Category = Tile, meta = (DisplayName = "On Hover Start"))
	void BP_OnHoverEnd(ACSKPlayerController* Controller);

	/** Refreshes the board piece UI data for the hovering player */
	UFUNCTION(BlueprintCallable, Category = Tile)
	void RefreshHoveringPlayersBoardPieceUI();

private:

	/** Reference to the controller hovering this tile. We keep this
	here to notify the player of any changes to the occupant board piece */
	TWeakObjectPtr<ACSKPlayerController> HoveringPlayer;

	/** The current state of if this tile can be selected or not (used for visualization) */
	ETileSelectionState SelectionState;

public:

	/** Set a new board piece to occupy this tile. Get if setting piece was successful */
	bool SetBoardPiece(AActor* BoardPiece);

	/** Clears the board piece currently occupying this tile. Get if a board piece was cleared */
	bool ClearBoardPiece();

protected:

	/** Event for when a new board piece has been placed on this tile */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board|Tiles", meta = (DisplayName = "On Board Piece Set"))
	void BP_OnBoardPieceSet(AActor* NewBoardPiece);

	/** Event for when the occupant board piece has been cleared */
	UFUNCTION(BlueprintImplementableEvent, Category = "Board|Tiles", meta = (DisplayName = "On Board Piece Cleared"))
	void BP_OnBoardPieceCleared();

	/** Sets the board piece to occupy this tile */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_SetBoardPiece(AActor* BoardPiece);

	/** Clears the board piece occupying this tile */
	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClearBoardPiece();

public:

	/** If towers can be constructed on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	bool CanPlaceTowersOn() const;

	/** If this tile is currently occupied */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	virtual bool IsTileOccupied(bool bConsiderNull = true) const;

	/** If this tile is occupied by given player (as in the 
	board piece placed on this tile belongs to said player) */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	bool DoesPlayerOccupyTile(const ACSKPlayerState* Player) const;

public:

	/** Get the board piece placed on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	AActor* GetBoardPiece() const;

	/** Get the player state for the player whose board piece is on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	ACSKPlayerState* GetBoardPiecesOwner() const;

	/** Get the player ID of the player whose board piece is on this tile */
	int32 GetBoardPiecesOwnerPlayerID() const;

	/** Get the health component of the board piece on this tile */
	UFUNCTION(BlueprintPure, Category = "Board|Tiles")
	UHealthComponent* GetBoardPieceHealthComponent() const;

	/** Constructs and returns UI data about the current board piece occupant */\
	FBoardPieceUIData GetBoardPieceUIData() const;

private:

	/** The board piece currently on this tile */
	UPROPERTY(Transient)
	TScriptInterface<IBoardPieceInterface> PieceOccupant;

public:

	/** Refreshes this tiles highlight material */
	UFUNCTION(BlueprintCallable, Category = "Board|Tiles")
	void RefreshHighlightMaterial();
};
