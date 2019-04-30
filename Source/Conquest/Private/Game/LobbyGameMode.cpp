// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "LobbyGameState.h"
#include "LobbyHUD.h"
#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"

#include "CSKGameInstance.h"

#define LOCTEXT_NAMESPACE "LobbyGameMode"

ALobbyGameMode::ALobbyGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	GameStateClass = ALobbyGameState::StaticClass();
	HUDClass = ALobbyHUD::StaticClass();
	PlayerControllerClass = ALobbyPlayerController::StaticClass();
	PlayerStateClass = ALobbyPlayerState::StaticClass();

	DefaultPlayerName = LOCTEXT("DefaultPlayerName", "Sorcerer");
	bUseSeamlessTravel = true;

	StartCountdownTime = 5.f;
	bTravellingToMatch = false;

	#if WITH_EDITORONLY_DATA
	P1AssignedColor = FColor::Red;
	P2AssignedColor = FColor::Green;
	#endif
}

void ALobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// Fix the size of the players array, as it won't be changing throughout the lobby
	Players.SetNum(CSK_MAX_NUM_PLAYERS);

	Super::InitGame(MapName, Options, ErrorMessage);
}

void ALobbyGameMode::InitGameState()
{
	Super::InitGameState();

	// We only need to se this once
	ALobbyGameState* LobbyGameState = Cast<ALobbyGameState>(GameState);
	if (LobbyGameState)
	{
		LobbyGameState->SetStartCountdownTime(StartCountdownTime);
		LobbyGameState->SetSelectableMaps(SelectableMaps);

		// Choose a random map as the default selectable map
		if (SelectableMaps.Num() > 0)
		{
			int32 Index = FMath::RandRange(0, SelectableMaps.Num() - 1);
			LobbyGameState->SetSelectedMap(SelectableMaps[Index]);
		}
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	// We need to set which player this is before continuing the login
	{
		if (GetPlayer1Controller() != nullptr)
		{
			// We should only ever have two players
			if (!ensure(!GetPlayer2Controller()))
			{
				UE_LOG(LogConquest, Error, TEXT("More than 2 players have joined the lobby match even though the max is 2 players"));
			}
			else
			{
				SetPlayerWithID(CastChecked<ALobbyPlayerController>(NewPlayer), 1);
			}
		}
		else
		{
			SetPlayerWithID(CastChecked<ALobbyPlayerController>(NewPlayer), 0);
		}

	}

	Super::PostLogin(NewPlayer);
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	if (IsMatchValid())
	{
		// If we are changing hosts (Player with ID of zero is host)
		bool bHostChange = false;

		if (Players[0] == Exiting)
		{
			// Set old player as the new 'host'
			Players[0] = Players[1];
			Players[1] = nullptr;

			bHostChange = true;
		}
		else if (ensure(Players[1] == Exiting))
		{
			Players[1] = nullptr;
		}

		// Notify the remaining player about the change
		ALobbyPlayerController* Controller = Players[0];
		if (Controller)
		{
			// First update players ID to now be host player
			if (bHostChange)
			{
				Controller->CSKPlayerID = 0;
				
				ALobbyPlayerState* PlayerState = Controller->GetLobbyPlayerState();
				if (PlayerState)
				{
					PlayerState->SetCSKPlayerID(0);
				}
			}

			// Refresh the clients lobby member player states
			SendPlayerLobbyPlayerStates(Controller);
		}

		// We could possibly be counting down, we have to make sure to stop
		ALobbyGameState* LobbyGameState = Cast<ALobbyGameState>(GameState);
		if (LobbyGameState)
		{
			LobbyGameState->StopStartCountdown();
		}
	}

	Super::Logout(Exiting);
}

bool ALobbyGameMode::IsMatchValid() const
{
	if (Players.Num() > 0)
	{
		return (Players[0] != nullptr && Players[1] != nullptr);
	}
	
	return false;
}

void ALobbyGameMode::SetPlayerWithID(ALobbyPlayerController* Controller, int32 PlayerID)
{
	if (!Controller || Controller->CSKPlayerID >= 0)
	{
		return;
	}

	if (Players.IsValidIndex(PlayerID))
	{
		// Space might already be occupied
		if (Players[PlayerID] == nullptr)
		{
			// Save reference to this player
			Players[PlayerID] = Controller;
			Controller->CSKPlayerID = PlayerID;

			ALobbyPlayerState* PlayerState = Controller->GetLobbyPlayerState();
			if (PlayerState)
			{
				PlayerState->SetCSKPlayerID(PlayerID);

				#if WITH_EDITORONLY_DATA
				PlayerState->SetAssignedColor(PlayerID == 0 ? P1AssignedColor : P2AssignedColor);
				#endif
			}
		}
	}
}

void ALobbyGameMode::NotifyPlayerReady(ALobbyPlayerController* Player, bool bIsReady)
{
	if (bTravellingToMatch)
	{
		return;
	}

	if (Player)
	{
		ALobbyPlayerState* PlayerState = Player->GetLobbyPlayerState();
		if (PlayerState && PlayerState->IsPlayerReady() != bIsReady)
		{
			PlayerState->SetIsReady(bIsReady);

			// Inform game state to update, this will either start or cancel the countdown
			ALobbyGameState* LobbyGameState = Cast<ALobbyGameState>(GameState);
			if (LobbyGameState)
			{
				LobbyGameState->UpdateStartCountdownStatus();
			}
		}
	}
}

void ALobbyGameMode::NotifyChangeColor(ALobbyPlayerController* Player, const FColor& Color)
{
	if (bTravellingToMatch)
	{
		return;
	}

	if (Player)
	{
		// We don't need to continue if no change will occur
		ALobbyPlayerState* PlayerState = Player->GetLobbyPlayerState();
		if (!PlayerState || PlayerState->GetAssignedColor() == Color)
		{
			return;
		}

		// We don't want both players to have the same color
		ALobbyPlayerController* OpponentsController = Players[FMath::Abs(Player->CSKPlayerID - 1)];
		if (OpponentsController)
		{
			ALobbyPlayerState* OpponentsState = OpponentsController->GetLobbyPlayerState();
			if (OpponentsState && OpponentsState->GetAssignedColor() == Color)
			{
				return;
			}
		}

		PlayerState->SetAssignedColor(Color);
	}
}

void ALobbyGameMode::NotifySelectMap(const FMapSelectionDetails& MapDetails)
{
	if (bTravellingToMatch)
	{
		return;
	}

	if (MapDetails.IsValid())
	{
		ALobbyGameState* LobbyGameState = Cast<ALobbyGameState>(GameState);
		if (LobbyGameState)
		{
			LobbyGameState->SetSelectedMap(MapDetails);
		}
	}
}

void ALobbyGameMode::NotifyCountdownFinished()
{
	// We are already on our way to the map
	if (bTravellingToMatch)
	{
		return;
	}

	// All players should be ready, double check in-case
	if (!AreAllPlayersReady())
	{
		return;
	}

	ALobbyGameState* LobbyGameState = Cast<ALobbyGameState>(GameState);
	if (LobbyGameState)
	{
		const FMapSelectionDetails& PendingMap = LobbyGameState->GetSelectedMap();
		if (PendingMap.IsValid())
		{
			FString LevelName = PendingMap.MapFileName;
			TArray<FString> Options{ "gamemode='Blueprint'/Game/Game/Blueprints/BP_CSKGameMode.BP_CSKGameMode'" };

			// Travel to the level
			UCSKGameInstance::ServerTravelToLevel(this, LevelName, Options);
			bTravellingToMatch = true;
		}
		else
		{
			UE_LOG(LogConquest, Error, TEXT("Unable to Travel to Match as Selected Map is Invalid"));
		}
	}
}

bool ALobbyGameMode::AreAllPlayersReady() const
{
	if (IsMatchValid())
	{
		for (ALobbyPlayerController* Controller : Players)
		{
			ALobbyPlayerState* PlayerState = Controller->GetLobbyPlayerState();
			if (!PlayerState || !PlayerState->IsPlayerReady())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void ALobbyGameMode::RefreshPlayersLobbyPlayerStates(ALobbyPlayerController* Controller)
{
	SendPlayerLobbyPlayerStates(Controller);
}

void ALobbyGameMode::SendPlayerLobbyPlayerStates(ALobbyPlayerController* Controller) const
{
	if (Controller)
	{
		ALobbyPlayerController* HostController = GetPlayer1Controller();
		ALobbyPlayerState* HostState = HostController ? HostController->GetLobbyPlayerState() : nullptr;

		ALobbyPlayerController* GuestController = GetPlayer2Controller();
		ALobbyPlayerState* GuestState = GuestController ? GuestController->GetLobbyPlayerState() : nullptr;

		Controller->Client_RecieveLobbyMembersPlayerStats(HostState, GuestState);
	}
}

#undef LOCTEXT_NAMESPACE