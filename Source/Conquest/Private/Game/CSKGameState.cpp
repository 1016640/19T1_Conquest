// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"

#include "BoardManager.h"
#include "Engine/World.h"

#include "Kismet/KismetSystemLibrary.h"

ACSKGameState::ACSKGameState()
{
	BoardManager = nullptr;
}

void ACSKGameState::HandleBeginPlay()
{
	Super::HandleBeginPlay();
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
		HandleStateChange(NewState);
	}
}

void ACSKGameState::NotifyWaitingForPlayers()
{
	UWorld* World = GetWorld();
	World->GetWorldSettings()->NotifyBeginPlay();
}

void ACSKGameState::NotifyMatchStart()
{
	UWorld* World = GetWorld();
	World->GetWorldSettings()->NotifyMatchStarted();

	if (HasAuthority())
	{
		bReplicatedHasBegunPlay = true;
	}

	UKismetSystemLibrary::PrintString(this, FString("The match has started!"), true, false, FLinearColor::Green, 10.f);
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

void ACSKGameState::OnRep_MatchState()
{
	HandleStateChange(MatchState);
}

void ACSKGameState::HandleStateChange(ECSKMatchState NewState)
{
	// Actions performed during the pre match phase should always be run before entering other states.
	// Doing this helps people who joined late to correctly catch up to the action
	if (NewState == ECSKMatchState::PreMatchWait || PreviousState == ECSKMatchState::EnteringMap)
	{
		NotifyWaitingForPlayers();
	}

	switch (NewState)
	{
		case ECSKMatchState::InProgress:
		{
			NotifyMatchStart();
			break;
		}
		case ECSKMatchState::PostMatchWait:
		{
			NotifyMatchFinished();
			break;
		}
		case ECSKMatchState::LeavingMap:
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
	PreviousState = NewState;
}
