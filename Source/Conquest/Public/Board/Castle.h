// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Pawn.h"
#include "BoardPieceInterface.h"
#include "Castle.generated.h"

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
};
