// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UCSKGameInstance::UCSKGameInstance()
{
	// Initialize session bindings
	{
		OnCreateSessionComplete = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyCreateSessionComplete);
		OnStartSessionComplete = FOnStartSessionCompleteDelegate::CreateUObject(this, &UCSKGameInstance::NotifyStartSessionCompleted);
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

bool UCSKGameInstance::IsValidNameForSession(const FName& SessionName) const
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
		UE_LOG(LogConquest, Error, TEXT("UCSKGameInstance::CreateSession: No OnlineSubsystem Found"));
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

void UCSKGameInstance::NotifyStartSessionCompleted(FName SessionName, bool bWasSuccessful)
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
	}
}
