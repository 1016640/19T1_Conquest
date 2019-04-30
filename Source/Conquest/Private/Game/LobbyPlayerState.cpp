// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyPlayerState.h"

ALobbyPlayerState::ALobbyPlayerState()
{
	CSKPlayerID = -1;
	bIsPlayerReady = false;
	AssignedColor = FColor(80, 50, 20); // Bronze
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, CSKPlayerID);
	DOREPLIFETIME(ALobbyPlayerState, bIsPlayerReady);
	DOREPLIFETIME(ALobbyPlayerState, AssignedColor);
}

void ALobbyPlayerState::SetCSKPlayerID(int32 InPlayerID)
{
	if (HasAuthority())
	{
		CSKPlayerID = InPlayerID;
	}
}

void ALobbyPlayerState::SetIsReady(bool bInIsReady)
{
	if (HasAuthority())
	{
		bIsPlayerReady = bInIsReady;
	}
}

void ALobbyPlayerState::SetAssignedColor(FColor InColor)
{
	if (HasAuthority())
	{
		AssignedColor = InColor;
	}
}
