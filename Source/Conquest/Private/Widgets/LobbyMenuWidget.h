// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Blueprint/UserWidget.h"
#include "LobbyMenuWidget.generated.h"

class ALobbyPlayerState;

/**
 * Specialized widget to for the lobby
 */
UCLASS(abstract)
class ULobbyMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Notify that lobby members player states have been refreshed.
	Neither state is guaranteed to be valid, so be sure to check */
	UFUNCTION(BlueprintImplementableEvent)
	void OnLobbyMembersRefreshed(ALobbyPlayerState* HostState, ALobbyPlayerState* GuestState);

	/** Notify that the selected map has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSelectedMapChanged(const FMapSelectionDetails& MapDetails);
};
