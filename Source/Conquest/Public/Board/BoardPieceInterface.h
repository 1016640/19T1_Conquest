// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BoardPieceInterface.generated.h"

UINTERFACE(MinimalAPI)
class UBoardPieceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface that must be implemented for pieces that can be placed onto tiles.
 */
class CONQUEST_API IBoardPieceInterface
{
	GENERATED_BODY()

};
