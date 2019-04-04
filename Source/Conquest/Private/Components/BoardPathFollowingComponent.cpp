// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardPathFollowingComponent.h"
#include "Tile.h"

UBoardPathFollowingComponent::UBoardPathFollowingComponent()
{
	CurrentTileIndex = 0;
}

void UBoardPathFollowingComponent::FollowPath(const FBoardPath& InPath)
{
	if (InPath.IsValid())
	{
		Reset();

		BoardPath = InPath;
		Status = EPathFollowingStatus::Moving;
	}
}

void UBoardPathFollowingComponent::Reset()
{
	Super::Reset();

	BoardPath.Reset();
	CurrentTileIndex = 0;
}

// TODO: Tidy
void UBoardPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
	const FVector CurrentTarget = BoardPath[CurrentTileIndex]->GetActorLocation();

	bIsDecelerating = false;

	const bool bAccelerationBased = MovementComp->UseAccelerationForPathFollowing();
	if (bAccelerationBased)
	{
		CurrentMoveInput = (CurrentTarget - CurrentLocation).GetSafeNormal();

		if (MoveSegmentStartIndex >= DecelerationSegmentIndex)
		{
			const FVector PathEnd = BoardPath[BoardPath.Num() - 1]->GetActorLocation();
			const float DistToEndSq = FVector::DistSquared(CurrentLocation, PathEnd);
			const bool bShouldDecelerate = DistToEndSq < FMath::Square(CachedBrakingDistance);
			if (bShouldDecelerate)
			{
				bIsDecelerating = true;

				const float SpeedPct = FMath::Clamp(FMath::Sqrt(DistToEndSq) / CachedBrakingDistance, 0.0f, 1.0f);
				CurrentMoveInput *= SpeedPct;
			}
		}

		PostProcessMove.ExecuteIfBound(this, CurrentMoveInput);
		MovementComp->RequestPathMove(CurrentMoveInput);
	}
	else
	{
		FVector MoveVelocity = (CurrentTarget - CurrentLocation) / DeltaTime;

		const int32 LastSegmentStartIndex = FMath::Max(0, BoardPath.Num() - 2);
		const bool bNotFollowingLastSegment = (MoveSegmentStartIndex < LastSegmentStartIndex);

		PostProcessMove.ExecuteIfBound(this, MoveVelocity);
		MovementComp->RequestDirectMove(MoveVelocity, bNotFollowingLastSegment);
	}
}

// TODO: tidy
void UBoardPathFollowingComponent::UpdatePathSegment()
{
	if (!BoardPath.IsValid())
	{
		OnPathFinished(EPathFollowingResult::Aborted, FPathFollowingResultFlags::InvalidPath);
		return;
	}

	const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
	const bool bCanUpdateState = HasMovementAuthority();

	// Are we allowed to control the agents movements?
	if (bCanUpdateState && Status == EPathFollowingStatus::Moving)
	{
		const FVector CurrentSegmentLocation = BoardPath[CurrentTileIndex]->GetActorLocation();
		const int32 LastSegment = BoardPath.Num() - 1;

		if (FVector::DistSquaredXY(CurrentLocation, CurrentSegmentLocation) < 1.f)
		{
			if (CurrentTileIndex == LastSegment)
			{
				OnSegmentFinished();
				OnPathFinished(EPathFollowingResult::Success, FPathFollowingResultFlags::None);

				OnBoardPathFinished.Broadcast(BoardPath[LastSegment]);
			}
			else
			{
				++CurrentTileIndex;

				// Move onto next tile
				ATile* NextTargetTile = BoardPath[CurrentTileIndex];
				if (!NextTargetTile)
				{
					OnPathFinished(EPathFollowingResult::Aborted, FPathFollowingResultFlags::InvalidPath);

					OnBoardPathFinished.Broadcast(BoardPath[LastSegment]);
					return;
				}

				FVector NextSegmentLocation = NextTargetTile->GetActorLocation();

				CurrentDestination.Set(NextTargetTile, NextSegmentLocation);
				MoveSegmentDirection = (NextSegmentLocation - CurrentSegmentLocation).GetSafeNormal();
				UpdateMoveFocus();

				OnSegmentFinished();

				OnBoardSegmentCompleted.Broadcast(NextTargetTile);
			}
		}
	}
}
