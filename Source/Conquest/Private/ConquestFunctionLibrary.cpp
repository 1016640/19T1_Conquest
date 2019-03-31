// Fill out your copyright notice in the Description page of Project Settings.

#include "ConquestFunctionLibrary.h"
#include "Board/BoardManager.h"
#include "Game/CSKGameInstance.h"
#include "Game/CSKGameMode.h"
#include "Game/CSKGameState.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

bool UConquestFunctionLibrary::IsWithEditor()
{
	#if WITH_EDITOR
	return true;
	#else
	return false;
	#endif
}

bool UConquestFunctionLibrary::IsOnMobile()
{
	#if PLATFORM_IOS || PLATFORM_ANDROID
	return true;
	#else
	return false;
	#endif
}

UCSKGameInstance* UConquestFunctionLibrary::GetCSKGameInstance(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetGameInstance<UCSKGameInstance>() : nullptr;
}

ACSKGameMode* UConquestFunctionLibrary::GetCSKGameMode(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetAuthGameMode<ACSKGameMode>() : nullptr;
}

ACSKGameState* UConquestFunctionLibrary::GetCSKGameState(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetGameState<ACSKGameState>() : nullptr;
}

ABoardManager* UConquestFunctionLibrary::GetMatchBoardManger(const UObject* WorldContextObject)
{
	const ACSKGameState* GameState = GetCSKGameState(WorldContextObject);
	return GameState ? GameState->GetBoardManager() : nullptr;
}