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

bool ACastleAIController::FollowPath(const FBoardPath& InPath)
{
	UBoardPathFollowingComponent* BoardFollowComponent = GetBoardPathFollowingComponent();
	if (BoardFollowComponent && BoardFollowComponent->GetStatus() != EPathFollowingStatus::Moving)
	{
		return BoardFollowComponent->FollowPath(InPath);
	}

	return false;
}

void ACastleAIController::OnBoardPathSegmentCompleted(ATile* SegmentTile)
{

}

void ACastleAIController::OnBoardPathCompleted(ATile* DestinationTile)
{
	UE_LOG(LogConquest, Log, TEXT("Castle %s has finished moving!"), *GetCastle()->GetFName().ToString());
}

ACastle* ACastleAIController::GetCastle() const
{
	return Cast<ACastle>(GetPawn());
}

UBoardPathFollowingComponent* ACastleAIController::GetBoardPathFollowingComponent() const
{
	return Cast<UBoardPathFollowingComponent>(GetPathFollowingComponent());
}
