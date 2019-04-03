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

	/** Performing a coin flip to decide who gets to go first */
	CoinFlip,

	/** The match is in progress */
	Running,

	/** Match has finished via a win or lose condition. We are now having a small cooldown (actors are still ticking) */
	WaitingPostMatch,

	/** We are leaving the game and returning to lobby */
	LeavingGame,

	/** Match was abandoned due to unseen circumstances */
	Aborted
};

/** The current phase of a round */
UENUM(BlueprintType)
enum class ECSKRoundState : uint8
{
	/** Players are collecting resources */
	CollectionPhase,

	/** The player who won the dice roll is performing their action phase */
	FirstActionPhase,

	/** The player who lost the dice roll is performing their action phase */
	SecondActionPhase,

	/** Any tiles with actions can now perform them */
	EndRoundPhase
};

/** The state a player is in duing their action phase */
UENUM(BlueprintType)
enum class ECSKPlayerActionState : uint8
{
	/** Player isn't in any state */
	None,

	/** Player is currently selecting a tile to move to */
	MoveCastle,

	/** Player is currently selecting a tile to build a tower on */
	BuildTowers,

	/** Player is currently selecting a spell to cast */
	CastSpell
};