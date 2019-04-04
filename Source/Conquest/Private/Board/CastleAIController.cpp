// Fill out your copyright notice in the Description page of Project Settings.

#include "CastleAIController.h"
#include "Castle.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "BoardPathFollowingComponent.h"

ACastleAIController::ACastleAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBoardPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	UBoardPathFollowingComponent* BoardFollowComponent = GetBoardPathFollowingComponent();
	if (BoardFollowComponent)
	{
		BoardFollowComponent->OnBoardSegmentCompleted.AddUObject(this, &ACastleAIController::OnBoardPathSegmentCompleted);
		BoardFollowComponent->OnBoardPathFinished.AddUObject(this, &ACastleAIController::OnBoardPathCompleted);
	}
}

void ACastleAIController::FollowPath(const FBoardPath& InPath)
{
	UBoardPathFollowingComponent* BoardFollowComponent = GetBoardPathFollowingComponent();
	if (BoardFollowComponent && BoardFollowComponent->GetStatus() != EPathFollowingStatus::Moving)
	{
		// TODO: Have follow path return true or false (for if path follow started successfully
		// if started, we want to notify the first tile that it is un-occupied
		BoardFollowComponent->FollowPath(InPath);
	}
}

void ACastleAIController::OnBoardPathSegmentCompleted(ATile* SegmentTile)
{
	// Keep track of how many tiles we have moved
	ACSKPlayerState* State = PlayerOwner ? PlayerOwner->GetCSKPlayerState() : nullptr;
	if (State)
	{
		State->IncrementTilesTraversed();
	}

	// TODO: tell game mode we have completed a tile segment and to check win condition
}

void ACastleAIController::OnBoardPathCompleted(ATile* DestinationTile)
{
	// Keep track of how many tiles we have moved
	ACSKPlayerState* State = PlayerOwner ? PlayerOwner->GetCSKPlayerState() : nullptr;
	if (State)
	{
		State->IncrementTilesTraversed();
	}

	// TODO: Would want to notify game state or game mode that castle move has finished
	// TODO: tell game mode we have completed a tile segment and to check win condition
	UE_LOG(LogConquest, Log, TEXT("Castle %s has finished moving!"), *GetCastle()->GetFName().ToString());
}

ACastle* ACastleAIController::GetCastle() const
{
	return Cast<ACastle>(GetPawn());
}

UBoardPathFollowingComponent * ACastleAIController::GetBoardPathFollowingComponent() const
{
	return Cast<UBoardPathFollowingComponent>(GetPathFollowingComponent());
}
