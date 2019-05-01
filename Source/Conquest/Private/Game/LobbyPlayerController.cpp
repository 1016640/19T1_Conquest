// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "LobbyHUD.h"
#include "LobbyGameMode.h"
#include "LobbyGameState.h"

#include "TimerManager.h"

ALobbyPlayerController::ALobbyPlayerController()
{
	bShowMouseCursor = true;
	CSKPlayerID = -1;
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		UWorld* World = GetWorld();
		ALobbyGameState* GameState = World ? World->GetGameState<ALobbyGameState>() : nullptr;

		FMapSelectionDetails SelectedMap;

		// We want to listen out for when the selected map changes
		if (GameState)
		{
			SelectedMap = GameState->GetSelectedMap();
			GameState->OnSelectedMapChanged.AddDynamic(this, &ALobbyPlayerController::OnSelectedMapChanged);		
		}

		// We can also update the selected map at this time
		ALobbyHUD* LobbyHUD = GetLobbyHUD();
		if (LobbyHUD)
		{
			LobbyHUD->NotifySelectedMapChanged(SelectedMap);
		}

		// Cheap way to wait for player states to replicate
		FTimerHandle TempHandle;
		FTimerManager& TimerManager = GetWorldTimerManager();
		TimerManager.SetTimer(TempHandle, this, &ALobbyPlayerController::Server_RefreshLobbyMembersPlayerStates, 1.f, false);
	}
}

void ALobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsLocalPlayerController())
	{
		UWorld* World = GetWorld();
		ALobbyGameState* GameState = World ? World->GetGameState<ALobbyGameState>() : nullptr;

		if (GameState)
		{
			GameState->OnSelectedMapChanged.RemoveDynamic(this, &ALobbyPlayerController::OnSelectedMapChanged);
		}
	}
}

void ALobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerController, CSKPlayerID);
}

ALobbyPlayerState* ALobbyPlayerController::GetLobbyPlayerState() const
{
	return GetPlayerState<ALobbyPlayerState>();
}

ALobbyHUD* ALobbyPlayerController::GetLobbyHUD() const
{
	return Cast<ALobbyHUD>(MyHUD);
}

void ALobbyPlayerController::ToggleIsReady()
{
	if (IsLocalPlayerController())
	{
		ALobbyPlayerState* LobbyState = GetLobbyPlayerState();
		if (LobbyState)
		{
			Server_NotifyToggleReady(!LobbyState->IsPlayerReady());
		}
	}
}

void ALobbyPlayerController::ChangePlayerColor(FColor Color)
{
	if (IsLocalPlayerController())
	{
		Server_NotifyChangeColor(Color);
	}
}

void ALobbyPlayerController::SelectMap(int32 MapIndex)
{
	if (IsLocalPlayerController())
	{
		if (IsLobbyHost())
		{
			Server_NotifySelectMap(MapIndex);
		}
	}
}

bool ALobbyPlayerController::Server_NotifyToggleReady_Validate(bool bIsReady)
{
	return true;
}

void ALobbyPlayerController::Server_NotifyToggleReady_Implementation(bool bIsReady)
{
	UWorld* World = GetWorld();
	ALobbyGameMode* GameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;

	if (GameMode)
	{
		GameMode->NotifyPlayerReady(this, bIsReady);
	}
}

bool ALobbyPlayerController::Server_NotifyChangeColor_Validate(FColor Color)
{
	return true;
}

void ALobbyPlayerController::Server_NotifyChangeColor_Implementation(FColor Color)
{
	UWorld* World = GetWorld();
	ALobbyGameMode* GameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;

	if (GameMode)
	{
		GameMode->NotifyChangeColor(this, Color);
	}
}

bool ALobbyPlayerController::Server_NotifySelectMap_Validate(int32 MapIndex)
{
	return true;
}

void ALobbyPlayerController::Server_NotifySelectMap_Implementation(int32 MapIndex)
{
	UWorld* World = GetWorld();
	ALobbyGameMode* GameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;

	if (GameMode)
	{
		GameMode->NotifySelectMap(MapIndex);
	}
}

void ALobbyPlayerController::OnSelectedMapChanged(const FMapSelectionDetails& MapDetails)
{
	ALobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->NotifySelectedMapChanged(MapDetails);
	}
}

bool ALobbyPlayerController::Server_RefreshLobbyMembersPlayerStates_Validate()
{
	return true;
}

void ALobbyPlayerController::Server_RefreshLobbyMembersPlayerStates_Implementation()
{
	UWorld* World = GetWorld();
	ALobbyGameMode* GameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;

	if (GameMode)
	{
		GameMode->RefreshPlayersLobbyPlayerStates(this);
	}
}

void ALobbyPlayerController::Client_RecieveLobbyMembersPlayerStats_Implementation(ALobbyPlayerState* HostState, ALobbyPlayerState* GuestState)
{
	ALobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->NotifyLobbyMembersRefreshed(HostState, GuestState);
	}
}
