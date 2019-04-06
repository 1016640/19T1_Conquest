// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerState.h"
#include "CSKGameMode.h"
#include "Tower.h"

ACSKPlayerState::ACSKPlayerState()
{
	CSKPlayerID = -1;
	Castle = nullptr;

	AssignedColor = FColor(80, 50, 20); // Bronze
	Gold = 0;
	Mana = 0;
	CachedNumLegendaryTowers = 0;

	TilesTraversedThisRound = 0;
	TotalTilesTraversed = 0;
	TotalTowersBuilt = 0;
	TotalLegendaryTowersBuilt = 0;
}

void ACSKPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACSKPlayerState, CSKPlayerID);
	DOREPLIFETIME(ACSKPlayerState, Castle);
		
	DOREPLIFETIME(ACSKPlayerState, Gold);
	DOREPLIFETIME(ACSKPlayerState, Mana);
	DOREPLIFETIME(ACSKPlayerState, AssignedColor);

	DOREPLIFETIME_CONDITION(ACSKPlayerState, TilesTraversedThisRound, COND_OwnerOnly);
}

void ACSKPlayerState::SetCSKPlayerID(int32 InPlayerID)
{
	if (HasAuthority())
	{
		CSKPlayerID = InPlayerID;
	}
}

void ACSKPlayerState::SetCastle(ACastle* InCastle)
{
	if (HasAuthority())
	{
		Castle = InCastle;
	}
}

void ACSKPlayerState::GiveResources(int32 InGold, int32 InMana)
{
	SetResources(Gold + InGold, Mana + InMana);
}

void ACSKPlayerState::SetResources(int32 InGold, int32 InMana)
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this); // TODO: get rid of this and have game mode calc and set it
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
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this); // TODO: get rid of this and have game mode calc and set it
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
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this); // TODO: get rid of this and have game mode calc and set it
	if (GameMode)
	{
		Mana = GameMode->ClampManaToLimit(Amount);
	}
}

void ACSKPlayerState::AddTower(ATower* InTower)
{
	if (HasAuthority() && InTower)
	{
		OwnedTowers.Add(InTower);

		// Recalculate the cached counts we have
		UpdateTowerCounts();

		if (InTower->IsLegendaryTower())
		{
			++TotalLegendaryTowersBuilt;
		}
		else
		{
			++TotalTowersBuilt;
		}
	}
}

void ACSKPlayerState::RemoveTower(ATower* InTower)
{
	if (HasAuthority())
	{
		// We can avoid recalc if we don't remove anything
		if (OwnedTowers.Remove(InTower) > 0)
		{
			// Recalculate the cached counts we have
			UpdateTowerCounts();
		}
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

int32 ACSKPlayerState::GetNumOwnedTowerDuplicates(TSubclassOf<ATower> Tower) const
{
	const int32* Num = CachedUniqueTowerCount.Find(Tower);
	if (Num != nullptr)
	{
		return *Num;
	}

	return 0;
}

void ACSKPlayerState::OnRep_OwnedTowers()
{
	UpdateTowerCounts();
}

void ACSKPlayerState::UpdateTowerCounts()
{
	CachedNumLegendaryTowers = 0;
	CachedUniqueTowerCount.Reset();

	for (ATower* Tower : OwnedTowers)
	{
		if (!ensure(Tower))
		{
			continue;
		}

		if (Tower->IsLegendaryTower())
		{
			++CachedNumLegendaryTowers;
		}

		UClass* StaticTower = Tower->StaticClass();
		check(StaticTower);

		if (CachedUniqueTowerCount.Contains(StaticTower))
		{
			CachedUniqueTowerCount[StaticTower]++;
		}
		else
		{
			CachedUniqueTowerCount.Add(StaticTower, 1);
		}
	}
}

void ACSKPlayerState::IncrementTilesTraversed()
{
	if (HasAuthority())
	{
		++TilesTraversedThisRound;
		++TotalTilesTraversed;

		#if WITH_EDITOR
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			// Warning check
			if (!GameMode->IsCountWithinTileTravelLimits(TilesTraversedThisRound))
			{
				UE_LOG(LogConquest, Warning, TEXT("Player %s has exceeded amount of tiles that can be traversed this round! "
					"Max Tiles per turn = %i, Tiles moved this turn = %i"), *GetPlayerName(), GameMode->GetMaxTileMovementsPerTurn(), TilesTraversedThisRound);
			}
		}
		#endif
	}
}

void ACSKPlayerState::ResetTilesTraversed()
{
	if (HasAuthority())
	{
		TilesTraversedThisRound = 0;
	}
}