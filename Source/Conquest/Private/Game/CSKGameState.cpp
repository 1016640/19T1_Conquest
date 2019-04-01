// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"

#include "BoardManager.h"
#include "Engine/World.h"

ACSKGameState::ACSKGameState()
{
	BoardManager = nullptr;
}

void ACSKGameState::HandleBeginPlay()
{
	// Retrieve board manager before handling begin
	// play as some classes might depend on it
	BoardManager = FindBoardManager();

	Super::HandleBeginPlay();
}

void ACSKGameState::OnRep_ReplicatedHasBegunPlay()
{
	if (bReplicatedHasBegunPlay && Role != ROLE_Authority)
	{
		BoardManager = FindBoardManager();
	}

	Super::OnRep_ReplicatedHasBegunPlay();
}

ABoardManager* ACSKGameState::GetBoardManager(bool bErrorCheck) const
{
	if (bErrorCheck && !ensure(BoardManager))
	{
		UE_LOG(LogConquest, Error, TEXT("CSKGameState::GetBoardManager: BoardManager is null"));
	}

	return BoardManager;
}

ABoardManager* ACSKGameState::FindBoardManager() const
{
	UWorld* World = GetWorld();
	for (TActorIterator<ABoardManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}
