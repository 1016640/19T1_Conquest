// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"

#include "BoardManager.h"
#include "Engine/World.h"

ACSKGameState::ACSKGameState()
{

}

void ACSKGameState::HandleBeginPlay()
{
	UWorld* World = GetWorld();
	for (TActorIterator<ABoardManager> It(World); It; ++It)
	{
		BoardManager = *It;
	}

	if (!BoardManager)
	{
		UE_LOG(LogConquest, Error, TEXT("Unable to obtain board manager for current session!"));
	}

	Super::HandleBeginPlay();
}

ABoardManager* ACSKGameState::GetBoardManager(bool bErrorCheck) const
{
	if (bErrorCheck && !ensure(BoardManager))
	{
		UE_LOG(LogConquest, Error, TEXT("CSKGameState::GetBoardManager: BoardManager is null"));
	}

	return BoardManager;
}
