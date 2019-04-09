// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Pawn.h"
#include "BoardPieceInterface.h"
#include "Castle.generated.h"

class UFloatingPawnMovement;
class USkeletalMeshComponent;

/**
 * Castles are towers that have the capability of moving. Castles are indirectly
 * controlled by players, but instead controlled by the CastleAIController
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACastle : public APawn, public IBoardPieceInterface
{
	GENERATED_BODY()

public:

	ACastle();

public:

	// Begin IBoardPiece Interface
	virtual void SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState) override;
	virtual ACSKPlayerState* GetBoardPieceOwnerPlayerState() const override { return OwnerPlayerState; }
	virtual void PlacedOnTile(ATile* Tile) override;
	virtual void RemovedOffTile() override;
	virtual UHealthComponent* GetHealthComponent() const override { return HealthTracker; }
	// End IBoardPiece Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Get skeletal mesh component */
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** Get pawn movement component */
	FORCEINLINE UFloatingPawnMovement* GetPawnMovement() const { return PawnMovement; }

private:

	/** Animated skeletal mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	/** Basic Movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UFloatingPawnMovement* PawnMovement;

	/** Health component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthTracker;

protected:

	/** Event for when the castle has been placed on given tile */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Placed on Tile"))
	void BP_OnPlacedOnTile(ATile* Tile);

	/** Event for when the castle has moved off of given tile */
	UFUNCTION(BlueprintImplementableEvent, Category = BoardPiece, meta = (DisplayName = "On Removed from Tile"))
	void BP_OnRemovedFromTile(ATile* Tile);

protected:

	/** The player state of the player who owns this castle */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = BoardPiece)
	ACSKPlayerState* OwnerPlayerState;

public:

	/** Get the tile we are currently on */
	FORCEINLINE ATile* GetCachedTile() const { return CachedTile; }

private:

	/** The tile we are currently on, this is cached for quick access to it */
	UPROPERTY(Transient, DuplicateTransient)
	ATile* CachedTile;
};
