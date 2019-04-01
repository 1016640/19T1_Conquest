// Fill out your copyright notice in the Description page of Project Settings.

#include "Castle.h"
#include "CastleAIController.h"

ACastle::ACastle()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessAI = EAutoPossessAI::Spawned;
	AIControllerClass = ACastleAIController::StaticClass();
}

