// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Navigation/PathFollowingComponent.h"
#include "BoardTypes.h"
#include "BoardPathFollowingComponent.generated.h"

// TODO: Make a custom UActor component, or maybe insert into a custom movement component?
// The only reason I'm keeping it as a path following component cause 1, its a component to follow paths with and 2,
// AI controllers already have a member variable for path following components, so it would feel iffy to have two different
// components that do the same thing

/** Event for when we have reached new segment of the path */
DECLARE_MULTICAST_DELEGATE_OneParam(FBoardMoveComplete, ATile* /** DestinationTile */);

/**
 * Simple path following component that utilizes the board instead of the navmesh.
 * This component does not use any functionality from the previous besides a few variables.
 */
UCLASS()
class CONQUEST_API UBoardPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()

public:

	UBoardPathFollowingComponent();

public:

	/** Follows the given path */
	void FollowPath(const FBoardPath& InPath);

protected:

	// Begin UPathFollowingComponent Interface
	virtual void Reset() override;
	virtual void FollowPathSegment(float DeltaTime) override;
	virtual void UpdatePathSegment() override;
	// End UPathFollowingComponent Interface

public:

	/** Event for board segment completed */
	FBoardMoveComplete OnBoardSegmentCompleted;

	/** Event for board path completed */
	FBoardMoveComplete OnBoardPathFinished;

protected:

	/** The path we are currently following (if any) */
	FBoardPath BoardPath;

	/** The index for the tile we are currently travelling to */
	int32 CurrentTileIndex;
};
