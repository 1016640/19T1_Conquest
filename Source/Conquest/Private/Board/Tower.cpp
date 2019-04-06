// Fill out your copyright notice in the Description page of Project Settings.

#include "Tower.h"

ATower::ATower()
{
	PrimaryActorTick.bCanEverTick = false;

	OwnerPlayerState = nullptr;
}

void ATower::SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState)
{
	if (HasAuthority())
	{
		OwnerPlayerState = InPlayerState;
	}
}

void ATower::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATower, OwnerPlayerState, COND_InitialOnly);
}
