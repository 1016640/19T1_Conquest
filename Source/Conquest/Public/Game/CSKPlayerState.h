// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerState.h"
#include "CSKPlayerState.generated.h"

/**
 * Tracks states and stats for a player
 */
UCLASS()
class CONQUEST_API ACSKPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ACSKPlayerState();

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

protected:

	/** The amount of gold this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	uint8 Gold;

	/** The amount of mana this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	uint8 Mana;

	/** This players assigned color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Resources)
	FColor AssignedColor;

private:

	/** The amount of tiles this player has travelled */
	UPROPERTY()
	int32 TilesTraversed;
};
