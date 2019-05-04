// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Engine/GameInstance.h"
#include "CSKGameInstance.generated.h"

class FOnlineSessionSettings;

/**
 * Instance for handling data used throughout CSK. Provides functions for creating,
 * joining and ending online sessions specified to the criteria of CSK
 */
UCLASS()
class CONQUEST_API UCSKGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UCSKGameInstance();

public:

	/** Server travels to given level with options */
	UFUNCTION(BlueprintCallable, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static bool ServerTravelToLevel(const UObject* WorldContextObject, FString LevelName, const TArray<FString>& Options);

public:

	/** If the given name is valid for a sessions name */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool IsValidNameForSession(const FName& SessionName) const;

public:

	/** Creates a new session where which player will host */
	bool CreateSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence);

protected:

	/** Notify that create session request has completed */
	virtual void NotifyCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Notify that start session request has completed */
	virtual void NotifyStartSessionCompleted(FName SessionName, bool bWasSuccessful);

protected:

	/** Stores the settings for the current session */
	TSharedPtr<FOnlineSessionSettings> SessionSettings;

	/** Delegate for when creating a session has completed */
	FOnCreateSessionCompleteDelegate OnCreateSessionComplete;

	/** Delegate for when starting a session had completed */
	FOnStartSessionCompleteDelegate OnStartSessionComplete;

	/** Handles to session callbacks */
	FDelegateHandle Handle_OnCreateSession;
	FDelegateHandle Handle_OnStartSession;

	// temp
	UFUNCTION(BlueprintImplementableEvent)
	void DestroySessionAfterMatchFinished();
};
