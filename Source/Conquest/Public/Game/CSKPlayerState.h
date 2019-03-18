// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

	/** The amount of gold this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Resources)
	int32 Gold : 8;

	/** The amount of mana this player owns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Resources)
	int32 Mana : 8;

	/** This players assigned color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Resources)
	FColor AssignedColor;
};
