// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "BoardManager.generated.h"

class ATile;

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
};

