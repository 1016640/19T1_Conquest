// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConquestFunctionLibrary.h"
#include "EngineUtils.h"
#include "UnrealNetwork.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConquest, Log, All);
DECLARE_STATS_GROUP(TEXT("Conquest"), STATGROUP_Conquest, STATCAT_Advanced);

/** The max number of clients allowed in a session (including local host) */
#define CSK_MAX_SESSION_PLAYERS 2