// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConquestFunctionLibrary.h"
#include "EngineUtils.h"
#include "UnrealNetwork.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConquest, Log, All);
DECLARE_STATS_GROUP(TEXT("Conquest"), STATGROUP_Conquest, STATCAT_Advanced);

/** The max number of clients allowed in a session (including local host) */
#define CSK_MAX_NUM_PLAYERS 2

/** The current state of the match taking place. This
works similar to how AGameMode works (see GameMode.h) */
UENUM(BlueprintType)
enum class ECSKMatchState : uint8
{
	/** We are entering the map in which to play */
	EnteringMap,

	/** We are waiting for all clients to be ready (actors are already ticking) */
	PreMatchWait,

	/** The match is in progress */
	InProgress,

	/** Match has finished via a win or lose condition. We are now waiting before exiting (actors are still ticking) */
	PostMatchWait,

	/** We are leaving the map and returning to lobby */
	LeavingMap,

	/** Match was aborted due to unseen circumstances */
	Aborted
};