// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "LobbyHUD.h"
#include "LobbyGameMode.h"
#include "LobbyGameState.h"

ALobbyPlayerController::ALobbyPlayerController()
{
	bShowMouseCursor = true;
	CSKPlayerID = -1;
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
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

void ALobbyPlayerController::SelectMap(const FMapSelectionDetails& MapDetails)
{
	if (IsLocalPlayerController())
	{
		if (IsLobbyHost())
		{
			Server_NotifySelectMap(MapDetails);
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

bool ALobbyPlayerController::Server_NotifySelectMap_Validate(const FMapSelectionDetails& MapDetails)
{
	return true;
}

void ALobbyPlayerController::Server_NotifySelectMap_Implementation(const FMapSelectionDetails& MapDetails)
{
	UWorld* World = GetWorld();
	ALobbyGameMode* GameMode = World ? World->GetAuthGameMode<ALobbyGameMode>() : nullptr;

	if (GameMode)
	{
		GameMode->NotifySelectMap(MapDetails);
	}
}
