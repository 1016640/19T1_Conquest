// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConquestFunctionLibrary.generated.h"

class ABoardManager;
class ACSKGameMode;
class ACSKGameState;
class UCSKGameInstance;

/**
 * Blueprint function library containing helper functions for Conquest based functionality
 */
UCLASS()
class CONQUEST_API UConquestFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	/** If game is executing in editor */
	UFUNCTION(BlueprintPure, Category = Platform)
	static bool IsWithEditor();

	/** If game is running on a mobile platform */
	UFUNCTION(BlueprintPure, Category = Platform)
	static bool IsOnMobile();

public:

	/** Get CSK game instance */
	UFUNCTION(BlueprintPure, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static UCSKGameInstance* GetCSKGameInstance(const UObject* WorldContextObject);

	/** Get CSK game mode */
	UFUNCTION(BlueprintPure, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static ACSKGameMode* GetCSKGameMode(const UObject* WorldContextObject);

	/** Get CSK game state */
	UFUNCTION(BlueprintPure, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static ACSKGameState* GetCSKGameState(const UObject* WorldContextObject);

	/** Get the board manager for the current match (if any) */
	UFUNCTION(BlueprintPure, Category = Board, meta = (WorldContext = "WorldContextObject"))
	static ABoardManager* GetMatchBoardManager(const UObject* WorldContextObject, bool bWarnIfNull = true);

	/** Get the first board manager found in the level */
	UFUNCTION(BlueprintPure, Category = Board, meta = (WorldContext = "WorldContextObject"))
	static ABoardManager* FindMatchBoardManager(const UObject* WorldContextObject, bool bWarnIfNotFound = true);
};
