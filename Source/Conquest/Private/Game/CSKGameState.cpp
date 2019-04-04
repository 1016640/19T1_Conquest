// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "BoardManager.h"
#include "Engine/World.h"

#include "Kismet/KismetSystemLibrary.h"

ACSKGameState::ACSKGameState()
{
	BoardManager = nullptr;

	MatchState = ECSKMatchState::EnteringGame;
	RoundState = ECSKRoundState::Invalid;
	PreviousMatchState = MatchState;
	PreviousRoundState = RoundState;

	StartingPlayerID = 0;

	RoundsPlayed = 0;
}

void ACSKGameState::OnRep_ReplicatedHasBegunPlay()
{
	if (bReplicatedHasBegunPlay && Role != ROLE_Authority)
	{
		// We need to have a board manager, we will find one until we wait for the initial one to replicate
		if (!BoardManager)
		{
			BoardManager = UConquestFunctionLibrary::FindMatchBoardManager(this);
		}
	}

	Super::OnRep_ReplicatedHasBegunPlay();
}

void ACSKGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACSKGameState, BoardManager, COND_InitialOnly);
	DOREPLIFETIME(ACSKGameState, MatchState);
	DOREPLIFETIME(ACSKGameState, RoundState);

	DOREPLIFETIME(ACSKGameState, RoundsPlayed);
}

void ACSKGameState::SetMatchBoardManager(ABoardManager* InBoardManager)
{
	if (!BoardManager && HasAuthority())
	{
		BoardManager = InBoardManager;
	}
}

ABoardManager* ACSKGameState::GetBoardManager(bool bErrorCheck) const
{
	if (bErrorCheck && !ensure(BoardManager))
	{
		UE_LOG(LogConquest, Error, TEXT("CSKGameState::GetBoardManager: BoardManager is null"));
	}

	return BoardManager;
}

void ACSKGameState::SetMatchState(ECSKMatchState NewState)
{
	if (HasAuthority())
	{
		MatchState = NewState;
		HandleMatchStateChange(NewState);
	}
}

void ACSKGameState::NotifyWaitingForPlayers()
{
	UWorld* World = GetWorld();
	World->GetWorldSettings()->NotifyBeginPlay();
}

void ACSKGameState::NotifyPerformCoinFlip()
{
}

void ACSKGameState::NotifyMatchStart()
{
	UWorld* World = GetWorld();
	World->GetWorldSettings()->NotifyMatchStarted();

	if (HasAuthority())
	{
		// For now (Will move to perform coin flip)
		ACSKGameMode* CSKGameMode = Cast<ACSKGameMode>(AuthorityGameMode);
		if (CSKGameMode)
		{
			StartingPlayerID = CSKGameMode->CoinFlip() ? 0 : 1;
		}
		else
		{
			StartingPlayerID = UConquestFunctionLibrary::CoinFlip() ? 0 : 1;
		}

		bReplicatedHasBegunPlay = true;
	}
}

void ACSKGameState::NotifyMatchFinished()
{
}

void ACSKGameState::NotifyPlayersLeaving()
{
}

void ACSKGameState::NotifyMatchAbort()
{
}

void ACSKGameState::NotifyCollectionPhaseStart()
{
}

void ACSKGameState::NotifyFirstCollectionPhaseStart()
{
}

void ACSKGameState::NotifySecondCollectionPhaseStart()
{
}

void ACSKGameState::NotifyEndRoundPhaseStart()
{
}

void ACSKGameState::OnRep_MatchState()
{
	HandleMatchStateChange(MatchState);
}

void ACSKGameState::HandleMatchStateChange(ECSKMatchState NewState)
{
	// Actions performed during the pre match phase should always be run before entering other states.
	// Doing this helps people who joined late to correctly catch up to the action
	if (NewState == ECSKMatchState::WaitingPreMatch || PreviousMatchState == ECSKMatchState::EnteringGame)
	{
		NotifyWaitingForPlayers();
	}

	switch (NewState)
	{
		case ECSKMatchState::Running:
		{
			NotifyMatchStart();
			break;
		}
		case ECSKMatchState::WaitingPostMatch:
		{
			NotifyMatchFinished();
			break;
		}
		case ECSKMatchState::LeavingGame:
		{
			NotifyPlayersLeaving();
			break;
		}
		default:
		{
			break;
		}
	}

	// Setting it same as new state, as previous state will be valid
	// after being replicated from the server (or before this call)
	PreviousMatchState = NewState;
}

void ACSKGameState::OnRep_RoundState()
{
	HandleRoundStateChange(RoundState);
}

void ACSKGameState::HandleRoundStateChange(ECSKRoundState NewState)
{
	switch (NewState)
	{
		case ECSKRoundState::CollectionPhase:
		{
			NotifyCollectionPhaseStart();
			break;
		}
		case ECSKRoundState::FirstActionPhase:
		{
			NotifyFirstCollectionPhaseStart();
			break;
		}
		case ECSKRoundState::SecondActionPhase:
		{
			NotifySecondCollectionPhaseStart();
			break;
		}
		case ECSKRoundState::EndRoundPhase:
		{
			NotifyEndRoundPhaseStart();
			break;
		}
		default:
		{
			break;
		}
	}

	// Setting it same as new state, as previous state will be valid
	// after being replicated from the server (or before this call)
	PreviousMatchState = NewState;
}
