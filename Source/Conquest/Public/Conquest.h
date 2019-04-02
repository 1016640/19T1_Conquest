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

/** The max number of spectators that can watch the game */
#define CSK_MAX_NUM_SPECTATORS 4

/** The current state of the match taking place. This
works similar to how AGameMode works (see GameMode.h) */
UENUM(BlueprintType)
enum class ECSKMatchState : uint8
{
	/** Players are joining the game */
	EnteringGame,

	/** We are waiting for all clients to be ready (actors are already ticking) */
	WaitingPreMatch,

	/** The match is in progress */
	Running,

	/** Match has finished via a win or lose condition. We are now having a small cooldown (actors are still ticking) */
	WaitingPostMatch,

	/** We are leaving the game and returning to lobby */
	LeavingGame,

	/** Match was abandoned due to unseen circumstances */
	Aborted
};