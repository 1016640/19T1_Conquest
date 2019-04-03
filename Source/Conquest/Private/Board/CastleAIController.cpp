// Fill out your copyright notice in the Description page of Project Settings.

#include "CastleAIController.h"
#include "Castle.h"

#include "BoardPathFollowingComponent.h"

ACastleAIController::ACastleAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBoardPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	UBoardPathFollowingComponent* BoardFollowComponent = GetBoardPathFollowingComponent();
	if (BoardFollowComponent)
	{
		BoardFollowComponent->OnBoardPathFinished.AddUObject(this, &ACastleAIController::OnBoardPathCompleted);
	}
}

void ACastleAIController::FollowPath(const FBoardPath& InPath)
{
	UBoardPathFollowingComponent* BoardFollowComponent = GetBoardPathFollowingComponent();
	if (BoardFollowComponent && BoardFollowComponent->GetStatus() != EPathFollowingStatus::Moving)
	{
		BoardFollowComponent->FollowPath(InPath);
	}
}

void ACastleAIController::OnBoardPathCompleted(ATile* DestinationTile)
{
	// TODO: Would want to notify game state or game mode that castle move has finished
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
