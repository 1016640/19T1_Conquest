// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerStart.h"
#include "CSKPlayerStart.generated.h"

/**
 * Player starts used in CSK
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:

	/** Tag that should be used for the player start for spawning CSK pawns*/
	static const FName CSKPawnSpawnTag;

	/** Tag that should be used for finding the camera to use when flipping a coin */
	static const FName CoinFlipCameraTag;

public:

	#if WITH_EDITOR
	/** Sets this player start as a CSK pawn spawn */
	UFUNCTION(exec, meta = (DisplayName = "Set as CSK Pawn Spawn"))
	void SetStartAsCSKPawnSpawn();

	/** Sets this player start as a coin flip camera tag */
	UFUNCTION(exec, meta = (DisplayName = "Set as Coin Flip Camera"))
	void SetStartAsCoinFlipCamera();
	#endif
};
