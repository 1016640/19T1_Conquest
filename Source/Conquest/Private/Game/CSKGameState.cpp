// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "BoardManager.h"
#include "BoardPathFollowingComponent.h"
#include "CastleAIController.h"
#include "Engine/World.h"

ACSKGameState::ACSKGameState()
{
	BoardManager = nullptr;

	MatchState = ECSKMatchState::EnteringGame;
	RoundState = ECSKRoundState::Invalid;
	PreviousMatchState = MatchState;
	PreviousRoundState = RoundState;

	StartingPlayerID = 0;
	CurrentActionPhasePlayer = nullptr;

	RoundsPlayed = 0;

	bWaitingForCastleMove = false;
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
	PreviousRoundState = NewState;
}

void ACSKGameState::OnPlayerTravellingToTile(ACSKPlayerController* Controller, ATile* Goal)
{
	if (HasAuthority() && !ensure(Controller == CurrentActionPhasePlayer))
	{
		UE_LOG(LogConquest, Warning, TEXT("Game state was notified of player travelling but the player travelling "
			"is different from current action phase player"));
		return;
	}

	// Game mode is only exists on server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		// Get both players to focus on the castle as it moves
		TArray<ACSKPlayerController*> PlayerPCs = GameMode->GetCSKPlayerArray();
		for (ACSKPlayerController* PlayerPC : PlayerPCs)
		{
			PlayerPC->Client_FocusCastleDuringMovement(Controller->GetCastlePawn());
		}

		// Listen out for when castle finishes moving
		ACastleAIController* CastleController = Controller->GetCastleController();
		check(CastleController);
		
		UBoardPathFollowingComponent* FollowComp = CastleController->GetBoardPathFollowingComponent();
		Handle_PlayerTravelFinished = FollowComp->OnBoardPathFinished.AddUObject(this, &ACSKGameState::OnPlayerTravelFinished);

		bWaitingForCastleMove = true;
	}
}

void ACSKGameState::OnPlayerTravelFinished(ATile* DestinationTile)
{
	// Game mode is only exists on server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		// Revert both players to freely move their camera
		TArray<ACSKPlayerController*> PlayerPCs = GameMode->GetCSKPlayerArray();
		for (ACSKPlayerController* PlayerPC : PlayerPCs)
		{
			PlayerPC->Client_StopFocusingCastle();
		}

		check(CurrentActionPhasePlayer);

		// We no longer need to listen for path events
		ACastleAIController* CastleController = CurrentActionPhasePlayer->GetCastleController();
		check(CastleController);

		UBoardPathFollowingComponent* FollowComp = CastleController->GetBoardPathFollowingComponent();
		FollowComp->OnBoardPathFinished.Remove(Handle_PlayerTravelFinished);

		bWaitingForCastleMove = false;
		Handle_PlayerTravelFinished.Reset();
	}
}
