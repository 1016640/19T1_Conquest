// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "CSKPawnMovement.generated.h"

/**
 * Movement component to be used by CSK pawns
 */
UCLASS()
class CONQUEST_API UCSKPawnMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()
};
