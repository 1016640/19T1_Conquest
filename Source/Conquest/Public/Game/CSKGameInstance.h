// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Engine/GameInstance.h"
#include "CSKGameInstance.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSettings;

/** Wrapper for a search result with additional information */
USTRUCT(BlueprintType)
struct CONQUEST_API FConquestSearchResult
{
	GENERATED_BODY()

public:

	FConquestSearchResult()
	{

	}

	FConquestSearchResult(const FOnlineSessionSearchResult& InResult)
		: SearchResult(InResult)
	{

	}

public:

	/** Get the session itself */
	const FOnlineSession& GetSession() const;

	/** Get the name of the session */
	FString GetSessionName() const;

public:

	/** The result of the search */
	FOnlineSessionSearchResult SearchResult;
};

/** Delegate for when finding matches has finished */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFindMatchFinished, bool, bWasSuccessful, const TArray<FConquestSearchResult>&, SearchResults);

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

	// Begin UGameInstance Interface
	#if WITH_EDITOR
	virtual FGameInstancePIEResult InitializeForPlayInEditor(int32 PIEInstanceIndex, const FGameInstancePIEParameters& Params);
	#endif
	// End UGameInstance Interface

public:

	/** Server travels to given level with options */
	UFUNCTION(BlueprintCallable, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static bool ServerTravelToLevel(const UObject* WorldContextObject, FString LevelName, const TArray<FString>& Options);

public:

	/** Get the name of the session of a search result */
	UFUNCTION(BlueprintPure, Category = Online)
	static FString GetSessionName(const FConquestSearchResult& SearchResult);

public:

	/** Hosts a match. Uses given name as the session name */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool HostMatch(FName MatchName, bool bIsLAN);

	/** Finds potential matches to join */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool FindMatches(bool bIsLAN);

	/** Joins the match with given name */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool JoinMatch(const FConquestSearchResult& SearchResult);

	/** Leaves the current match. Destroys it if host */
	UFUNCTION(BlueprintCallable, Category = CSK)
	bool DestroyMatch();

public:

	/** If the given name is valid for a sessions name */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = CSK)
	bool IsValidNameForSession(const FName& SessionName) const;

	/** Get the name of the current match */
	UFUNCTION(BlueprintPure, Category = CSK)
	const FName& GetMatchName() const { return MatchSessionName; }

public:

	/** Event fired after match search has finished */
	UPROPERTY(BlueprintAssignable)
	FFindMatchFinished OnMatchesFound;

private:

	/** The name of the match we either are hosting or have joined */
	UPROPERTY(Transient)
	FName MatchSessionName;

protected:

	/** Creates a new session to host */
	bool InternalCreateSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence);

	/** Finds online sessions matching specifications */
	bool InternalFindSessions(TSharedPtr<const FUniqueNetId> UserId, bool bIsLAN, bool bIsPresence);

	/** Joins the session with given name */
	bool InternalJoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult);

	/** Destroys the current session thats active */
	bool InternalDestroySession(FName SessionName);

	/** Notify that create session request has completed */
	virtual void NotifyCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Notify that start session request has completed */
	virtual void NotifyStartSessionComplete(FName SessionName, bool bWasSuccessful);

	/** Notify that find sessions request has completed */
	virtual void NotifyFindSessionsComplete(bool bWasSuccessful);

	/** Notify that join session request has completed */
	virtual void NotifyJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Notify that destroy session request has completed */
	virtual void NotifyDestroySessionComplete(FName SessionName, bool bWasSuccessful);

protected:

	/** Stores the settings for the current session */
	TSharedPtr<FOnlineSessionSettings> SessionSettings;

	/** Stores the settings for the current session search */
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	/** Delegate for when creating a session has completed */
	FOnCreateSessionCompleteDelegate OnCreateSessionComplete;

	/** Delegate for when starting a session has completed */
	FOnStartSessionCompleteDelegate OnStartSessionComplete;

	/** Delegate for when searching for sessions has completed */
	FOnFindSessionsCompleteDelegate OnFindSessionsComplete;

	/** Delegate for when joining a session has completed */
	FOnJoinSessionCompleteDelegate OnJoinSessionComplete;

	/** Delegate for when destroying a session has completed */
	FOnDestroySessionCompleteDelegate OnDestroySessionComplete;

	/** Handles to session callbacks */
	FDelegateHandle Handle_OnCreateSession;
	FDelegateHandle Handle_OnStartSession;
	FDelegateHandle Handle_OnFindSessions;
	FDelegateHandle Handle_OnJoinSession;
	FDelegateHandle Handle_OnDestroySession;
};
