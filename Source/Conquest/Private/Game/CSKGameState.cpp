// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "BoardManager.h"
#include "BoardPathFollowingComponent.h"
#include "CastleAIController.h"
#include "Engine/World.h"

// for now
#include "Kismet/KismetSystemLibrary.h"

ACSKGameState::ACSKGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BoardManager = nullptr;

	MatchState = ECSKMatchState::EnteringGame;
	RoundState = ECSKRoundState::Invalid;
	PreviousMatchState = MatchState;
	PreviousRoundState = RoundState;

	ActivePhasePlayerID = -1;
	ActionPhaseTimeRemaining = -1.f;
	bFreezeActionPhaseTimer = false;

	RoundsPlayed = 0;
}

void ACSKGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ShouldCountdownActionPhase())
	{
		ActionPhaseTimeRemaining = FMath::Max(0.f, ActionPhaseTimeRemaining - DeltaTime);
		if (HasAuthority() && ActionPhaseTimeRemaining == 0.f)
		{
			// TODO: inform game mode that time has run out
		}

		FString TimeRem = FString("Time Remaining for Action Phase: ") + FString::SanitizeFloat(ActionPhaseTimeRemaining);
		UKismetSystemLibrary::PrintString(this, TimeRem, true, false, FLinearColor::Green, 0.f);
	}
}

void ACSKGameState::OnRep_ReplicatedHasBegunPlay()
{
	if (bReplicatedHasBegunPlay)
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

	DOREPLIFETIME(ACSKGameState, ActivePhasePlayerID);
	DOREPLIFETIME(ACSKGameState, ActionPhaseTimeRemaining);
}

void ACSKGameState::SetMatchBoardManager(ABoardManager* InBoardManager)
{
	if (HasAuthority() && !BoardManager)
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

void ACSKGameState::SetRoundState(ECSKRoundState NewState)
{
	if (HasAuthority())
	{
		RoundState = NewState;
		HandleRoundStateChange(NewState);
	}
}

bool ACSKGameState::IsMatchInProgress() const
{
	return MatchState == ECSKMatchState::Running;
}

bool ACSKGameState::IsActionPhaseActive() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::FirstActionPhase || RoundState == ECSKRoundState::SecondActionPhase;
	}

	return false;
}

void ACSKGameState::NotifyWaitingForPlayers()
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();
}

void ACSKGameState::NotifyPerformCoinFlip()
{
}

void ACSKGameState::NotifyMatchStart()
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyMatchStarted();

	if (HasAuthority())
	{
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
	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		ACSKPlayerController* ActivePlayer = GameMode->GetActionPhaseActiveController();
		if (ActivePlayer)
		{
			ActivePhasePlayerID = ActivePlayer->CSKPlayerID;
		}

		ActionPhaseTimeRemaining = GameMode->GetActionPhaseTime();
	}

	SetFreezeActionPhaseTimer(false);
}

void ACSKGameState::NotifySecondCollectionPhaseStart()
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		ACSKPlayerController* ActivePlayer = GameMode->GetActionPhaseActiveController();
		if (ActivePlayer)
		{
			ActivePhasePlayerID = ActivePlayer->CSKPlayerID;
		}

		ActionPhaseTimeRemaining = GameMode->GetActionPhaseTime();
	}

	SetFreezeActionPhaseTimer(false);
}

void ACSKGameState::NotifyEndRoundPhaseStart()
{
	SetActorTickEnabled(false);
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

void ACSKGameState::AddBonusActionPhaseTime()
{
	if (IsActionPhaseTimed())
	{
		// Game mode only exists on the server
		ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			ActionPhaseTimeRemaining = FMath::Min(GameMode->GetActionPhaseTime(), ActionPhaseTimeRemaining + GameMode->GetBonusActionPhaseTime());
		}
	}
}

void ACSKGameState::HandleMoveRequestConfirmed()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		Multi_HandleMoveRequestConfirmed();
	}
}

void ACSKGameState::HandleMoveRequestFinished()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		AddBonusActionPhaseTime();

		Multi_HandleMoveRequestFinished();
	}
}

void ACSKGameState::HandleBuildRequestConfirmed(ATower* NewTower)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after having built a tower
		AddBonusActionPhaseTime();

		Multi_HandleBuildRequestConfirmed(NewTower);
	}
}

void ACSKGameState::Multi_HandleMoveRequestConfirmed_Implementation()
{
	SetFreezeActionPhaseTimer(true);
}

void ACSKGameState::Multi_HandleMoveRequestFinished_Implementation()
{
	SetFreezeActionPhaseTimer(false);
}

void ACSKGameState::Multi_HandleBuildRequestConfirmed_Implementation(ATower* NewTower)
{
}