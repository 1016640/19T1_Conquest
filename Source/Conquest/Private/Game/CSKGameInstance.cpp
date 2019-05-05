// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/OnlineReplStructs.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UCSKGameInstance::UCSKGameInstance()
{
	// Initialize session bindings
	{
		OnCreateSessionComplete = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyCreateSessionComplete);
		OnStartSessionComplete = FOnStartSessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyStartSessionComplete);
		OnFindSessionsComplete = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyFindSessionsComplete);
		OnJoinSessionComplete = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyJoinSessionComplete);
		OnDestroySessionComplete = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyDestroySessionComplete);
	}
}

bool UCSKGameInstance::ServerTravelToLevel(const UObject* WorldContextObject, FString LevelName, const TArray<FString>& Options)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		FString URL = LevelName;

		// Append each option to the URL
		for (const FString& Option : Options)
		{
			URL.Append("?" + Option);
		}

		bool bSuccess = World->ServerTravel(URL);
		if (!bSuccess)
		{
			UE_LOG(LogConquest, Warning, TEXT("Failed to server travel to level %s"), *LevelName);
		}

		return bSuccess;
	}

	return false;
}

bool UCSKGameInstance::HostMatch(FName MatchName, bool bIsLAN)
{
	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	check(LocalPlayer);

	return CreateSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), MatchName, bIsLAN, true);
}

bool UCSKGameInstance::FindMatches(bool bIsLAN)
{
	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	check(LocalPlayer);

	return FindSessions(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), bIsLAN, true);
}

bool UCSKGameInstance::JoinMatch(FName MatchName)
{
	const ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
	check(LocalPlayer);

	// TODO:

	//return JoinSession(LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId(), MatchName, NeedSearchResult)
	return false;
}

bool UCSKGameInstance::DestroyMatch()
{
	return DestroySession(MatchSessionName);
}

bool UCSKGameInstance::IsValidNameForSession_Implementation(const FName& SessionName) const
{
	return SessionName.IsValid() && !SessionName.IsNone();
}

bool UCSKGameInstance::CreateSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, bool bIsLAN, bool bIsPresence)
{
	if (!UserId.IsValid())
	{
		UE_LOG(LogConquest, Warning, TEXT("UCSKGameInstance::CreateSession: UserId is Invalid"));
		return false;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// We need the sessions interface in order to create sessions
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SessionSettings = MakeShareable(new FOnlineSessionSettings());

			// Dynamic Settings
			SessionSettings->bIsLANMatch = bIsLAN;
			SessionSettings->bUsesPresence = bIsPresence;

			// Static Settings
			SessionSettings->NumPublicConnections = CSK_MAX_NUM_PLAYERS;
			SessionSettings->NumPrivateConnections = 0;
			SessionSettings->bAllowInvites = true;
			SessionSettings->bAllowJoinInProgress = true;
			SessionSettings->bShouldAdvertise = true;
			SessionSettings->bAllowJoinViaPresence = true;
			SessionSettings->bAllowJoinViaPresenceFriendsOnly = false;

			// We also want a valid name
			if (!IsValidNameForSession(SessionName))
			{
				ULocalPlayer* const Player = GetFirstGamePlayer();
				SessionName = FName(*Player->GetNickname());
			}

			SessionSettings->Set(SETTING_MAPNAME, FString("L_Lobby"), EOnlineDataAdvertisementType::ViaOnlineService);

			// We want to be notified when the latent task has finished
			Handle_OnCreateSession = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionComplete);

			// This may return successful, but it doesn't determine if the latent task will also be successful
			return Sessions->CreateSession(*UserId, SessionName, *SessionSettings);
		}
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("UCSKGameInstance: No OnlineSubsystem Found"));
	}

	return false;
}

bool UCSKGameInstance::FindSessions(TSharedPtr<const FUniqueNetId> UserId, bool bIsLAN, bool bIsPresence)
{
	if (!UserId.IsValid())
	{
		UE_LOG(LogConquest, Warning, TEXT("UCSKGameInstance::FindSessions: UserId is Invalid"));
		return false;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// We need the sessions interface in order to find sessions
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SessionSearch = MakeShareable(new FOnlineSessionSearch());

			// Dynamic Settings
			SessionSearch->bIsLanQuery = bIsLAN;

			// Static Settings
			SessionSearch->MaxSearchResults = 10;
			SessionSearch->PingBucketSize = 50;

			// We only set presence query setting if desired
			if (bIsPresence)
			{
				SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bIsPresence, EOnlineComparisonOp::Equals);
			}

			// We want to be notified when the latent task has finished
			Handle_OnFindSessions = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsComplete);

			// This may return successful, but it doesn't determine if the latent task will also be successful
			return Sessions->FindSessions(*UserId, SessionSearch.ToSharedRef());
		}
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("UCSKGameInstance: No OnlineSubsystem Found"));
	}

	return false;
}

bool UCSKGameInstance::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	if (!UserId.IsValid())
	{
		UE_LOG(LogConquest, Warning, TEXT("UCSKGameInstance::FindSessions: UserId is Invalid"));
		return false;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// We need the sessions interface in order to join sessions
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// We want to be notified when the latent task has finished
			Handle_OnJoinSession = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionComplete);

			// This may return successful, but it doesn't determine if the latent task will also be successful
			return Sessions->JoinSession(*UserId, SessionName, SearchResult);
		}
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("UCSKGameInstance: No OnlineSubsystem Found"));
	}

	return false;
}

bool UCSKGameInstance::DestroySession(FName SessionName)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// We need the sessions interface in order to join sessions
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// We want to be notified when the latent task has finished
			Handle_OnDestroySession = Sessions->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionComplete);

			// This may return successful, but it doesn't determine if the latent task will also be successful
			return Sessions->DestroySession(SessionName);
		}
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("UCSKGameInstance: No OnlineSubsystem Found"));
	}

	return false;
}

void UCSKGameInstance::NotifyCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Always clear this after our session task has finished
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(Handle_OnCreateSession);
			
			// We can immediately start the session once we created it
			if (bWasSuccessful)
			{
				// We want to be notified when the latent task has finished
				Handle_OnStartSession = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionComplete);

				Sessions->StartSession(SessionName);
			}
		}
	}
}

void UCSKGameInstance::NotifyStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Always clear this after our session task has finished
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(Handle_OnStartSession);
		}
	}

	if (bWasSuccessful)
	{
		UGameplayStatics::OpenLevel(GetWorld(), "L_Lobby", true, "listen");

		// The match we are hosting, we need to save it to destroy it later
		MatchSessionName = SessionName;
	}
}

void UCSKGameInstance::NotifyFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Always clear this after our session task has finished
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(Handle_OnFindSessions);

			// We can immediately start the session once we created it
			const TArray<FOnlineSessionSearchResult>& SearchResults = SessionSearch->SearchResults;
			if (SearchResults.Num() > 0)
			{
				// TODO:
			}
		}
	}
}

void UCSKGameInstance::NotifyJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Always clear this after our session task has finished
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(Handle_OnJoinSession);

			if (Result == EOnJoinSessionCompleteResult::Success)
			{
				// The URL we need to travel to to sync up with session
				FString URL;

				// Have the local client travel to URL of the session
				APlayerController* LocalController = GetFirstLocalPlayerController();
				if (LocalController && Sessions->GetResolvedConnectString(SessionName, URL))
				{
					LocalController->ClientTravel(URL, ETravelType::TRAVEL_Absolute);
				}

				// The match we have joined, we need to save it to destroy it later
				MatchSessionName = SessionName;
			}
		}
	}
}

void UCSKGameInstance::NotifyDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			// Always clear this after our session task has finished
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(Handle_OnDestroySession);
		}
	}

	if (bWasSuccessful)
	{
		UGameplayStatics::OpenLevel(GetWorld(), "L_MainMenu", true);

		// Reset as we longer are in a match
		MatchSessionName = NAME_None;
	}
}
