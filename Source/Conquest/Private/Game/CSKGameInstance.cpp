// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

UCSKGameInstance::UCSKGameInstance()
{

}

// TODO: Move to function library?
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
