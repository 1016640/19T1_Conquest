// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerState.h"

ACSKPlayerState::ACSKPlayerState()
{
	Gold = 0;
	Mana = 0;

	// Bronze
	AssignedColor = FColor(80, 50, 20);
}

void ACSKPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACSKPlayerState, Gold);
	DOREPLIFETIME(ACSKPlayerState, Mana);
	DOREPLIFETIME(ACSKPlayerState, AssignedColor);
}
