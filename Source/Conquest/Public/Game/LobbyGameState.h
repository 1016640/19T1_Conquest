// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

/** Delegate for when the selected map has changed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSelectedMapChanged, const FMapSelectionDetails&, MapDetails);

/**
 * Tracks the state of the lobby
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	ALobbyGameState();

public:

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Sets the time we give players before we enter the match */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void SetStartCountdownTime(float CountdownTime);

	/** Set the maps the host is able to selected from */
	void SetSelectableMaps(const TArray<FMapSelectionDetails>& MapDetails);

	/** Sets the selected map, this should be a valid map */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void SetSelectedMap(int32 MapIndex);

	/** Get if all players are ready */
	UFUNCTION(BlueprintPure, Category = Lobby)
	bool AreAllPlayersReady() const;

public:

	/** Get the selected map */
	UFUNCTION(BlueprintPure, Category = Lobby)
	FMapSelectionDetails GetSelectedMap() const;

protected:

	/** Notify that all players ready has been replicated */
	UFUNCTION()
	void OnRep_bAllPlayersReady();

	/** Notify that selected map has been replicated */
	UFUNCTION()
	void OnRep_SelectedMap();

public:

	/** Event for when the selected map has changed */
	UPROPERTY(BlueprintAssignable)
	FSelectedMapChanged OnSelectedMapChanged;

protected:

	/** If all players are ready */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, ReplicatedUsing = OnRep_bAllPlayersReady, Category = Lobby)
	uint32 bAllPlayersReady : 1;

	/** The index of the currently selected map */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, ReplicatedUsing = OnRep_SelectedMap, Category = Lobby)
	int32 SelectedMapIndex;

	/** The time remaining before the match will start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Replicated, Category = Lobby)
	float StartCountdownTimeRemaining;

	/** Cached start countdown timer used to reset the countdown time whenever all players are ready */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	float StartCountdownTime;

	/** Cached maps the host is allowed to select from */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = Rules)
	TArray<FMapSelectionDetails> SelectableMaps;

public:

	/** Updates the start countdown status by checking if all players are ready */
	void UpdateStartCountdownStatus();

	/** Forcefully stops the start countdown if running */
	void StopStartCountdown();
};
