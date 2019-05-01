// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameState.h"
#include "LobbyGameMode.h"
#include "LobbyPlayerState.h"

ALobbyGameState::ALobbyGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	StartCountdownTime = 5.f;
	bAllPlayersReady = false;
}

void ALobbyGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// A pre-caution
	if (bAllPlayersReady)
	{
		StartCountdownTimeRemaining = FMath::Max(0.f, StartCountdownTimeRemaining - DeltaTime);
		if (HasAuthority() && StartCountdownTimeRemaining == 0.f)
		{
			// Notify the game mode to travel to new level
			ALobbyGameMode* LobbyGameMode = Cast<ALobbyGameMode>(AuthorityGameMode);
			if (LobbyGameMode)
			{
				LobbyGameMode->NotifyCountdownFinished();
			}
		}
	}
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameState, bAllPlayersReady);
	DOREPLIFETIME(ALobbyGameState, SelectedMapIndex);
	DOREPLIFETIME(ALobbyGameState, StartCountdownTimeRemaining);
	DOREPLIFETIME(ALobbyGameState, StartCountdownTime);

	DOREPLIFETIME_CONDITION(ALobbyGameState, SelectableMaps, COND_InitialOnly);
}

void ALobbyGameState::SetStartCountdownTime(float CountdownTime)
{
	if (HasAuthority())
	{
		StartCountdownTime = CountdownTime;
		StartCountdownTimeRemaining = FMath::Min(StartCountdownTime, StartCountdownTimeRemaining);
	}
}

void ALobbyGameState::SetSelectableMaps(const TArray<FMapSelectionDetails>& MapDetails)
{
	if (HasAuthority())
	{
		SelectableMaps = MapDetails;
	}
}

void ALobbyGameState::SetSelectedMap(int32 MapIndex)
{
	if (HasAuthority())
	{
		if (SelectableMaps.IsValidIndex(MapIndex))
		{
			SelectedMapIndex = MapIndex;
			OnRep_SelectedMap();
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ALobbyGameState::SetSelectedMap: Passed in map details is invalid. Make sure the map file name is correct"));
		}
	}
}

bool ALobbyGameState::AreAllPlayersReady() const
{
	// We need at least two players
	if (PlayerArray.Num() >= CSK_MAX_NUM_PLAYERS)
	{
		for (APlayerState* PlayerState : PlayerArray)
		{
			ALobbyPlayerState* LobbyState = Cast<ALobbyPlayerState>(PlayerState);
			if (!LobbyState || !LobbyState->IsPlayerReady())
			{
				return false;
			}
		}

		return true;
	}
	
	return false;
}

FMapSelectionDetails ALobbyGameState::GetSelectedMap() const
{
	if (SelectableMaps.IsValidIndex(SelectedMapIndex))
	{
		return SelectableMaps[SelectedMapIndex];
	}

	return FMapSelectionDetails();
}

void ALobbyGameState::OnRep_bAllPlayersReady()
{
	SetActorTickEnabled(bAllPlayersReady);

	if (bAllPlayersReady)
	{
		StartCountdownTimeRemaining = StartCountdownTime;
	}
}

void ALobbyGameState::OnRep_SelectedMap()
{
	if (SelectableMaps.IsValidIndex(SelectedMapIndex))
	{
		OnSelectedMapChanged.Broadcast(SelectableMaps[SelectedMapIndex]);
	}
	else
	{
		OnSelectedMapChanged.Broadcast(FMapSelectionDetails());
	}
}

void ALobbyGameState::UpdateStartCountdownStatus()
{
	if (HasAuthority())
	{
		bool bPlayersReady = AreAllPlayersReady();
		if (bAllPlayersReady != bPlayersReady)
		{
			bAllPlayersReady = bPlayersReady;
			OnRep_bAllPlayersReady();			
		}
	}
}

void ALobbyGameState::StopStartCountdown()
{
	if (HasAuthority() && bAllPlayersReady)
	{
		bAllPlayersReady = false;
		OnRep_bAllPlayersReady();
	}
}
