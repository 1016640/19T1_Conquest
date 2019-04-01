// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "CSKGameState.generated.h"

class ABoardManager;

/**
 * Tracks state of game and stats about the board
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	ACSKGameState();

public:

	// Begin AGameStateBase Interface
	virtual void HandleBeginPlay() override;
	// End AGameStateBase Interface

protected:

	// Begin AGameStateBase Interface
	virtual void OnRep_ReplicatedHasBegunPlay() override;
	// End AGameStateBase Interface

public:

	/** Get the games board manager */
	UFUNCTION(BlueprintPure, Category = CSK)
	ABoardManager* GetBoardManager(bool bErrorCheck = true) const;

private:

	/** Finds the board manager in the current level */
	ABoardManager* FindBoardManager() const;

private:

	/** The board of this match */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = CSK)
	ABoardManager* BoardManager;
};
