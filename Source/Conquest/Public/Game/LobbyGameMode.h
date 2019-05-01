// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

class ALobbyPlayerController;
class ALobbyPlayerState;
class ALobbyGameState;

using FLobbyPlayerControllerArray = TArray<ALobbyPlayerController*, TFixedAllocator<CSK_MAX_NUM_PLAYERS>>;

/**
 * Manages the state of the lobby
 */
UCLASS(config=Game, ClassGroup = (CSK))
class CONQUEST_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	ALobbyGameMode();

public:

	// Begin AGameModeBase Interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	// End AGameModeBase Interface

public:

	/** Checks if match in current state is valid and can continue */
	UFUNCTION(BlueprintPure, Category = Lobby)
	bool IsMatchValid() const;

public:

	/** Get player ones controller */
	UFUNCTION(BlueprintPure, Category = Lobby)
	ALobbyPlayerController* GetPlayer1Controller() const { return Players[0]; }

	/** Get player twos controller */
	UFUNCTION(BlueprintPure, Category = Lobby)
	ALobbyPlayerController* GetPlayer2Controller() const { return Players[1]; }

	/** Get both players in the array*/
	const FLobbyPlayerControllerArray& GetPlayers() const { return Players; }

private:

	/** Helper function for setting a player to ID */
	void SetPlayerWithID(ALobbyPlayerController* Controller, int32 PlayerID);

protected:

	/** Array containing both players in order */
	FLobbyPlayerControllerArray Players;

public:

	/** Notify that the given player has toggled their ready status */
	void NotifyPlayerReady(ALobbyPlayerController* Player, bool bIsReady);

	/** Notify that the given player has switched their assigned color */
	void NotifyChangeColor(ALobbyPlayerController* Player, const FColor& Color);

	/** Notify that the host has selected the map at given index */
	void NotifySelectMap(int32 MapIndex);

	/** Notify that the countdown has finished */
	void NotifyCountdownFinished();

private:

	/** The countdown has concluded and the match is starting */
	uint32 bTravellingToMatch : 1;

protected:

	/** The time we wait before starting the match when all players are ready. This
	gives some time for players to revert the ready status if something comes up */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Lobby, meta = (ClampMin = 0))
	float StartCountdownTime;

	/** The maps the host is allowed to select from. By default, a random map is chosen as the selected map */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Lobby)
	TArray<FMapSelectionDetails> SelectableMaps;

	/** The color to assign to the first joining player. This will
	be used for guest if host is using guests default color */
	UPROPERTY(EditAnywhere, Category = "Lobby|Debug")
	FColor HostAssignedColor;

	/** The color to assign to the guest joining the lobby */
	UPROPERTY(EditAnywhere, Category = "Lobby|Debug")
	FColor GuestAssignedColor;

public:

	/** Checks if both players are ready and the countdown can start */
	UFUNCTION(BlueprintPure, Category = Lobby)
	bool AreAllPlayersReady() const;

public:

	/** Request from a client to refresh the lobby member player states */
	void RefreshPlayersLobbyPlayerStates(ALobbyPlayerController* Controller);

private:

	/** Sends the lobby members player states to given player. This is used when
	players join or leave, in order for host and guest states be specified */
	void SendPlayerLobbyPlayerStates(ALobbyPlayerController* Controller) const;
};
