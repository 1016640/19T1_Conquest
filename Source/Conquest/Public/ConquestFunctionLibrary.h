// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConquestFunctionLibrary.generated.h"

class ABoardManager;
class ACSKGameMode;
class ACSKGameState;
class ACSKPlayerState;
class ATile;
class UCSKGameInstance;
class UUserWidget;

/** Information about a board piece to display to the user */
USTRUCT(BlueprintType)
struct CONQUEST_API FBoardPieceUIData
{
	GENERATED_BODY()

public:

	FBoardPieceUIData()
	{
		Owner = nullptr;
		BoardPiece = nullptr;
	}

public:

	/** The name of the board piece */
	UPROPERTY(BlueprintReadWrite)
	FText Name;

	/** A description of the board piece */
	UPROPERTY(BlueprintReadWrite)
	FText Description;

	/** The player that owns the board piece */
	UPROPERTY(BlueprintReadOnly)
	const ACSKPlayerState* Owner;

	/** The board piece associated with this data */
	UPROPERTY(BlueprintReadWrite)
	const AActor* BoardPiece;
};

/** Information about a building being damaged or healed */
USTRUCT(BlueprintType)
struct CONQUEST_API FHealthChangeReport
{
	GENERATED_BODY()

public:

	FHealthChangeReport()
		: Building(nullptr)
		, Owner(nullptr)
		, bIsCastle(false)
		, bWasDamaged(false)
		, bKilled(false)
		, Delta(0)
	{

	}

	FHealthChangeReport(AActor* InBuilding, ACSKPlayerState* InOwner, bool bInIsCastle, bool bInKilled, int32 InDelta)
		: Building(InBuilding)
		, Owner(InOwner)
		, bIsCastle(bInIsCastle)
		, bKilled(bInKilled)
		, Delta(FMath::Abs(InDelta))
	{
		ensure(InDelta != 0);

		// Damage results in a negative deltas, 
		// healing will produce positive deltas
		bWasDamaged = bKilled || InDelta < 0;
	}

public:

	/** The actor that had their health changed. This is either a tower or an castle.
	Be sure to check if valid, as this tower may be destroyed if it was killed */
	UPROPERTY(BlueprintReadOnly)
	AActor* Building;

	/** The owner of the tower */
	UPROPERTY(BlueprintReadOnly)
	ACSKPlayerState* Owner;

	/** If the effected actor was a castle or a tower */
	UPROPERTY(BlueprintReadOnly)
	uint8 bIsCastle : 1;

	/** If the change was a result of being damaged (If false, meant we were healed) */
	UPROPERTY(BlueprintReadOnly)
	uint8 bWasDamaged : 1;

	/** If this change resulted in actor being killed.
	Will always be false if WasDamaged is not true */
	UPROPERTY(BlueprintReadOnly)
	uint8 bKilled : 1;

	/** The delta of the change (This value will always be positive) */
	UPROPERTY(BlueprintReadOnly)
	int32 Delta;
};

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

public:

	/** Get if two tiles are within given amount of range of each other */
	UFUNCTION(BlueprintPure, Category = Board)
	static bool AreTilesWithingRange(const ATile* T1, const ATile* T2, int32 Range, int32& OutDistance);

	/** Accumalates all the deltas of the given health reports */
	UFUNCTION(BlueprintPure, Category = CSK)
	static int32 AccumulateHealthReportDeltas(const TArray<FHealthChangeReport>& Reports);

public:

	/** Adds widget to the viewport */
	UFUNCTION(BlueprintCallable, Category = UMG)
	static void AddWidgetToViewport(UUserWidget* Widget, int32 ZOrder = 0);

	/** Removes widget from its parent */
	UFUNCTION(BlueprintCallable, Category = UMG)
	static void RemoveWidgetFromParent(UUserWidget* Widget);

public:

	/** Converts seconds into an HH:MM:SS string */
	UFUNCTION(BlueprintPure, Category = String)
	static FString GetSecondsAsHourString(float Seconds);
};
