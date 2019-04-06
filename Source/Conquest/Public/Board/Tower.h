// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "BoardPieceInterface.h"
#include "Tower.generated.h"

/** 
 * Towers are board pieces that occupy a single tile and cannot move.
 */
UCLASS(abstract)
class CONQUEST_API ATower : public AActor, public IBoardPieceInterface
{
	GENERATED_BODY()
	
public:	

	ATower();

public:

	// Begin IBoardPiece Interface
	virtual void SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState) override;
	virtual ACSKPlayerState* GetBoardPieceOwnerPlayerState() const override { return OwnerPlayerState; }
	// End IBoardPiece Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

protected:

	/** The player state of the player who owns this tower */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = BoardPiece)
	ACSKPlayerState* OwnerPlayerState;

public:

	/** Get if this tower is a legendary tower */
	UFUNCTION(BlueprintPure, Category = "Board|Towers")
	bool IsLegendaryTower() const { return bIsLegendaryTower; }

protected:

	/** If this tower is a legendary tower */
	UPROPERTY(EditDefaultsOnly, Category = BoardPiece)
	uint8 bIsLegendaryTower : 1;
};
