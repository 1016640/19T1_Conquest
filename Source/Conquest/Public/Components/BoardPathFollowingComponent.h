// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Navigation/PathFollowingComponent.h"
#include "BoardTypes.h"
#include "BoardPathFollowingComponent.generated.h"

/**
 * Simple path following component that utilizes the board instead of the navmesh.
 */
UCLASS()
class CONQUEST_API UBoardPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:

	//UBoardPathFollowingComponent();

public:



protected:

	/** The path we are currently following (if any) */
	FBoardPath BoardPath;
};
