// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerState.h"
#include "CSKPlayerController.h"
#include "CSKGameMode.h"
#include "Tower.h"

ACSKPlayerState::ACSKPlayerState()
{
	CSKPlayerID = -1;
	Castle = nullptr;

	AssignedColor = FColor(80, 50, 20); // Bronze
	Gold = 0;
	Mana = 0;
	BonusTileMovements = 0;
	CachedNumLegendaryTowers = 0;
	MaxNumSpellUses = 1;
	bHasInfiniteSpellUses = false;

	TilesTraversedThisRound = 0;
	TotalTilesTraversed = 0;
	TotalTowersBuilt = 0;
	TotalLegendaryTowersBuilt = 0;
	SpellsCastThisRound = 0;
	TotalSpellsCast = 0;
	TotalQuickEffectSpellsCast = 0;
}

void ACSKPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACSKPlayerState, CSKPlayerID);
	DOREPLIFETIME(ACSKPlayerState, Castle);
		
	DOREPLIFETIME(ACSKPlayerState, AssignedColor);
	DOREPLIFETIME(ACSKPlayerState, Gold);
	DOREPLIFETIME(ACSKPlayerState, Mana);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, BonusTileMovements, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, OwnedTowers, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellCardDeck, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellCardsInHand, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, MaxNumSpellUses, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, bHasInfiniteSpellUses, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ACSKPlayerState, TilesTraversedThisRound, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellsCastThisRound, COND_OwnerOnly);
}

ACSKPlayerController* ACSKPlayerState::GetCSKPlayerController() const
{
	return CastChecked<ACSKPlayerController>(GetOwner());
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
	if (HasAuthority())
	{
		Gold = FMath::Max(0, InGold);
		Mana = FMath::Max(0, InMana);
	}
}

void ACSKPlayerState::AddGold(int32 Amount)
{
	SetGold(Gold + Amount);
}

void ACSKPlayerState::SetGold(int32 Amount)
{
	if (HasAuthority())
	{
		Gold = FMath::Max(0, Amount);
	}
}

void ACSKPlayerState::AddMana(int32 Amount)
{
	SetMana(Mana + Amount);
}

void ACSKPlayerState::SetMana(int32 Amount)
{
	if (HasAuthority())
	{
		Mana = FMath::Max(0, Amount);
	}
}

void ACSKPlayerState::AddBonusTileMovements(int32 Amount)
{
	SetBonusTileMovements(BonusTileMovements + Amount);
}

void ACSKPlayerState::SetBonusTileMovements(int32 Amount)
{
	if (HasAuthority())
	{
		BonusTileMovements = Amount;
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

void ACSKPlayerState::AddSpellUses(int32 Amount)
{
	SetSpellUses(MaxNumSpellUses + Amount);
}

void ACSKPlayerState::SetSpellUses(int32 Amount)
{
	if (HasAuthority())
	{
		MaxNumSpellUses = FMath::Max(0, Amount);
	}
}

void ACSKPlayerState::SetHasInfiniteSpellUses(bool bEnable)
{
	if (HasAuthority() && bHasInfiniteSpellUses != bEnable)
	{
		bHasInfiniteSpellUses = bEnable;
	}
}

TSubclassOf<USpellCard> ACSKPlayerState::PickupCardFromDeck()
{
	if (HasAuthority() && SpellCardDeck.IsValidIndex(0))
	{
		TSubclassOf<USpellCard> NextSpellCard = SpellCardDeck[0];
		SpellCardDeck.RemoveAt(0);

		return NextSpellCard;
	}

	// Implicit construction
	return nullptr;
}

void ACSKPlayerState::ResetSpellDeck(const TArray<TSubclassOf<USpellCard>>& Spells)
{
	if (HasAuthority())
	{
		SpellCardsInHand.Reset();
		SpellCardDeck = Spells;

		// Shuffle deck (mimics UKismetArrayLibrary shuffle function)
		int32 LastIndex = SpellCardDeck.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(i, LastIndex);
			if (i != Index)
			{
				SpellCardDeck.Swap(i, Index);
			}
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
	if (Tower)
	{		
		const int32* Num = CachedUniqueTowerCount.Find(Tower);
		if (Num != nullptr)
		{
			return *Num;
		}
	}

	return 0;
}

bool ACSKPlayerState::NeedsSpellDeckReshuffle() const
{
	if (SpellCardDeck.Num() == 0 && SpellCardsInHand.Num() == 0)
	{
		return true;
	}

	return false;
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

		TSubclassOf<ATower> TowerClass = Tower->GetClass();
		if (CachedUniqueTowerCount.Contains(TowerClass))
		{
			CachedUniqueTowerCount[TowerClass]++;
		}
		else
		{
			CachedUniqueTowerCount.Add(TowerClass, 1);
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
			if (!GameMode->IsCountWithinTileTravelLimits(TilesTraversedThisRound, BonusTileMovements))
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

void ACSKPlayerState::IncrementSpellsCast()
{
	if (HasAuthority())
	{
		++SpellsCastThisRound;
		++TotalSpellsCast;

		#if WITH_EDITOR
		// Warning check
		if (MaxNumSpellUses >= 0 && SpellsCastThisRound > MaxNumSpellUses)
		{
			UE_LOG(LogConquest, Warning, TEXT("Player %s has exceeded amount of spells they can ast this round! "
				"Max Spell Uses per Turn = %i, Spells Cast this turn = %i"), *GetPlayerName(), MaxNumSpellUses, SpellsCastThisRound);
		}
		#endif
	}
}

void ACSKPlayerState::ResetSpellsCast()
{
	if (HasAuthority())
	{
		SpellsCastThisRound = 0;
	}
}
