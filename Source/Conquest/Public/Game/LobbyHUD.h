// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

class ALobbyPlayerState;
class ULobbyMenuWidget;

/**
 * Manages displaying the HUD while in the lobby
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()
	
public:

	ALobbyHUD();

public:

	// Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor Interface

public:

	/** Refreshes the player list display */
	void NotifyLobbyMembersRefreshed(ALobbyPlayerState* HostState, ALobbyPlayerState* GuestState);

	/** Notify that the selected map has changed */
	void NotifySelectedMapChanged(const FMapSelectionDetails& MapDetails);

private:

	/** Get the lobby menu instance, creating it if needed */
	ULobbyMenuWidget* GetLobbyWidgetInstance();

protected:

	/** Instance of the lobby menu widget */
	UPROPERTY()
	ULobbyMenuWidget* LobbyWidgetInstance;

	/** The widget to use to display the lobby menu */
	UPROPERTY(EditAnywhere, Category = Lobby)
	TSubclassOf<ULobbyMenuWidget> LobbyWidgetTemplate;

private:

	/** If we have already logged the lobby menu
	widget template is invalid. This is to avoid spam*/
	uint8 bLoggedTemplateWarning : 1;
};
