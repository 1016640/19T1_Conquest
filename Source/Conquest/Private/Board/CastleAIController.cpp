// Fill out your copyright notice in the Description page of Project Settings.

#include "CastleAIController.h"
#include "Castle.h"

ACastle* ACastleAIController::GetCastle() const
{
	return Cast<ACastle>(GetPawn());
}