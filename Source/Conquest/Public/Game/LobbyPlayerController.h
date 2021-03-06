// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class ALobbyHUD;
class ALobbyPlayerState;

/**
 * Controller for handling communication events between players and the server
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	ALobbyPlayerController();

public:

	// Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** If this player is the host */
	UFUNCTION(BlueprintPure, Category = Lobby)
	bool IsLobbyHost() const { return CSKPlayerID == 0; }

public:

	/** This controllers player ID, this should never be altered */
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient, Replicated)
	int32 CSKPlayerID;

public:

	/** Get player state as lobby player state */
	UFUNCTION(BlueprintPure, Category = Lobby)
	ALobbyPlayerState* GetLobbyPlayerState() const;

	/** Get HUD as lobby HUD (only valid on clients) */
	UFUNCTION(BlueprintPure, Category = Lobby)
	ALobbyHUD* GetLobbyHUD() const;

public:

	/** Toggles this players ready status */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void ToggleIsReady();

	/** Attempts to set this players selected color */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void ChangePlayerColor(FColor Color);

	/** Sets map at given index as the selected map. This
	will only process if player is the host of the lobby */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void SelectMap(int32 MapIndex);

private:

	/** Notifies the server to toggle this players ready status */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifyToggleReady(bool bIsReady);

	/** Notifies the server to set this players color to given color */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifyChangeColor(FColor Color);

	/** Notifies the server to select the given map if we are host */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifySelectMap(int32 MapIndex);

private:

	/** Notify from the game state that the selected map has changed */
	UFUNCTION()
	void OnSelectedMapChanged(const FMapSelectionDetails& MapDetails);

public:

	/** Request the server to resend the host and party member player states */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RefreshLobbyMembersPlayerStates();

	/** Notify from the server about the host and party members player states */
	UFUNCTION(Client, Reliable)
	void Client_RecieveLobbyMembersPlayerStats(ALobbyPlayerState* HostState, ALobbyPlayerState* GuestState);
};
