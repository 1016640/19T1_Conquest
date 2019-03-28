// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerStart.h"
#include "CSKPlayerStart.generated.h"

/**
 * Player start used to track a tile to spawn and set a player on
 */
UCLASS()
class CONQUEST_API ACSKPlayerStart : public APlayerStart
{
	GENERATED_BODY()
	
public:

	static const FName Player1Start;
	static const FName Player2Start;
	static const FName InvalidStart;
};
