// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerState.h"
#include "CSKGameMode.h"

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

	DOREPLIFETIME_CONDITION(ACSKPlayerState, TilesTraversedThisRound, COND_OwnerOnly);
}

void ACSKPlayerState::GiveResources(int32 InGold, int32 InMana)
{
	SetResources(Gold + InGold, Mana + InMana);
}

void ACSKPlayerState::SetResources(int32 InGold, int32 InMana)
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		Gold = GameMode->ClampGoldToLimit(InGold);
		Mana = GameMode->ClampManaToLimit(InMana);
	}
}

void ACSKPlayerState::AddGold(int32 Amount)
{
	SetGold(Gold + Amount);
}

void ACSKPlayerState::SetGold(int32 Amount)
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		Gold = GameMode->ClampGoldToLimit(Amount);
	}
}

void ACSKPlayerState::AddMana(int32 Amount)
{
	SetMana(Mana + Amount);
}

void ACSKPlayerState::SetMana(int32 Amount)
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		Mana = GameMode->ClampManaToLimit(Amount);
	}
}

bool ACSKPlayerState::HasRequiredGold(int32 RequiredAmount) const
{
	return (Gold - RequiredAmount) >= 0;
}

bool ACSKPlayerState::HasRequiredMana(int32 RequiredAmount) const
{ 
	return (Mana - RequiredAmount) >= 0;
}

void ACSKPlayerState::IncrementTilesTraversed()
{
	++TilesTraversedThisRound;

	#if WITH_EDITOR
	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		// Warning check
		if (!GameMode->IsCountWithinTileTravelLimits(TilesTraversedThisRound))
		{
			UE_LOG(LogConquest, Warning, TEXT("Player %s has exceeded amount of tiles that can be traversed per round! "
				"Max Tiles per turn = %i, Tiles moved this turn = %i"), *GetPlayerName(), GameMode->GetMaxTileMovementsPerTurn(), TilesTraversedThisRound);
		}
	}
	#endif

	++TotalTilesTraversed;
}
