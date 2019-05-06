// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConquestFunctionLibrary.h"
#include "Engine.h"
#include "Online.h"
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
	/** No phase (used internally) */
	Invalid	UMETA(Hidden="true"),

	/** Players are collecting resources */
	CollectionPhase,

	/** The player who won the dice roll is performing their action phase */
	FirstActionPhase,

	/** The player who lost the dice roll is performing their action phase */
	SecondActionPhase,

	/** Any tiles with actions can now perform them */
	EndRoundPhase
};

/** The current mode the player is in during their action phase */
UENUM(BlueprintType)
enum class ECSKActionPhaseMode : uint8
{
	/** No action */
	None			= 0,

	/** Player is currently selecting a tile to move to */
	MoveCastle		= 1,

	/** Player is currently selecting a tile to build a tower on */
	BuildTowers		= 2,

	/** Player is currently selecting a spell to cast */
	CastSpell		= 4,

	/** All actions */
	All				= 7 UMETA(Hidden="true")
};

ENUM_CLASS_FLAGS(ECSKActionPhaseMode);

// TODO: This could be replaced by ESpellType
/** The context for a spells activiation */
UENUM(BlueprintType)
enum class EActiveSpellContext : uint8
{
	/** No context */
	None,

	/** Active player casting spell during action phase */
	Action,

	/** Active players opponents countering with a quick effect spell */
	Counter,

	/** Bonus spell cast due to element match */
	Bonus
};

/** The conditions to win a match of CSK */
UENUM(BlueprintType)
enum class ECSKMatchWinCondition : uint8
{
	/** A player has reached their opponents portal */
	PortalReached,

	/** A player has destroyed their opponents castle */
	CastleDestroyed,

	/** A player has surrendered */
	Surrender,

	/** Condition is unknown */
	Unknown
};