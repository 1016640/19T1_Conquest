// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerState.h"
#include "CSKPlayerController.h"
#include "CSKGameState.h"
#include "SpellCard.h"
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
	SpellDiscount = 0;

	TotalGoldCollected = 0;
	TotalManaCollected = 0;
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
	//DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellCardDeck, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellCardsInHand, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellCardsDiscarded, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, MaxNumSpellUses, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, bHasInfiniteSpellUses, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellDiscount, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ACSKPlayerState, TilesTraversedThisRound, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ACSKPlayerState, SpellsCastThisRound, COND_OwnerOnly);

	DOREPLIFETIME(ACSKPlayerState, TotalGoldCollected);
	DOREPLIFETIME(ACSKPlayerState, TotalManaCollected);
	DOREPLIFETIME(ACSKPlayerState, TotalTilesTraversed);
	DOREPLIFETIME(ACSKPlayerState, TotalTowersBuilt);
	DOREPLIFETIME(ACSKPlayerState, TotalLegendaryTowersBuilt);
	DOREPLIFETIME(ACSKPlayerState, TotalSpellsCast);
	DOREPLIFETIME(ACSKPlayerState, TotalQuickEffectSpellsCast);
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

void ACSKPlayerState::SetAssignedColor(FColor InColor)
{
	if (HasAuthority())
	{
		AssignedColor = InColor;
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
		TotalGoldCollected += FMath::Max(0, Amount - Gold);
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
		TotalManaCollected += FMath::Max(0, Amount - Mana);
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

void ACSKPlayerState::AddSpellDiscount(int32 Amount)
{
	SetSpellDiscount(SpellDiscount + Amount);
}

void ACSKPlayerState::SetSpellDiscount(int32 Amount)
{
	if (HasAuthority())
	{
		SpellDiscount = Amount;
	}
}

TSubclassOf<USpellCard> ACSKPlayerState::PickupCardFromDeck()
{
	if (HasAuthority() && SpellCardDeck.IsValidIndex(0))
	{
		TSubclassOf<USpellCard> NextSpellCard = SpellCardDeck[0];
		SpellCardsInHand.Add(NextSpellCard);
		SpellCardDeck.RemoveAt(0);

		return NextSpellCard;
	}

	// Implicit construction
	return nullptr;
}

void ACSKPlayerState::RemoveCardFromHand(TSubclassOf<USpellCard> Spell)
{
	if (HasAuthority())
	{
		if (SpellCardsInHand.Remove(Spell) > 0)
		{
			SpellCardsDiscarded.Add(Spell);
		}
	}
}

void ACSKPlayerState::ResetSpellDeck(const TArray<TSubclassOf<USpellCard>>& Spells)
{
	if (HasAuthority())
	{
		SpellCardsInHand.Reset();
		SpellCardsDiscarded.Reset();

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

bool ACSKPlayerState::HasRequiredMana(int32 RequiredAmount, bool bDiscount) const
{ 
	// Lowest amount of mana to spend is one
	RequiredAmount = bDiscount ? FMath::Max(1, RequiredAmount - SpellDiscount) : RequiredAmount;
	return Mana >= RequiredAmount;
}

bool ACSKPlayerState::GetDiscountedManaIfAffordable(int32 RequiredAmount, int32& OutAmount) const
{
	OutAmount = 0;

	// Lowest amount of mana to spend is one
	RequiredAmount = FMath::Max(1, RequiredAmount - SpellDiscount);
	if (Mana >= RequiredAmount)
	{
		OutAmount = RequiredAmount;
		return true;
	}

	return false;
}

bool ACSKPlayerState::HasRequiredManaPlusAdditionalAmount(int32 RequiredAmount, int32 MinAdditionalAmount) const
{
	// Lowest amount of mana to spend is one
	RequiredAmount = (RequiredAmount - SpellDiscount) + MinAdditionalAmount;
	return Mana >= RequiredAmount;
}

bool ACSKPlayerState::GetMaxAdditionalManaIfAffordable(int32 RequiredAmount, int32 MinAdditionalAmount, int32& OutMaxAmount) const
{
	OutMaxAmount = 0;

	// Lowest amount of mana to spend is one
	int32 MinRequiredAmount = (RequiredAmount - SpellDiscount) + MinAdditionalAmount;
	if (Mana >= MinRequiredAmount)
	{
		OutMaxAmount = Mana - RequiredAmount;
		return true;
	}

	return false;
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

bool ACSKPlayerState::CanCastAnotherSpell(bool bCheckCost) const
{
	if (bHasInfiniteSpellUses || SpellsCastThisRound < MaxNumSpellUses)
	{
		if (!bCheckCost)
		{
			return true;
		}

		return CanAffordSpellOfType(ESpellType::ActionPhase);
	}

	return false;
}

bool ACSKPlayerState::CanCastQuickEffectSpell() const
{
	return CanAffordSpellOfType(ESpellType::QuickEffect);
}

void ACSKPlayerState::GetSpellsPlayerCanCast(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const
{
	GetAffordableSpells(OutSpellCards, ESpellType::ActionPhase);
}

void ACSKPlayerState::GetQuickEffectSpellsPlayerCanCast(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const
{
	GetAffordableSpells(OutSpellCards, ESpellType::QuickEffect);
}

int32 ACSKPlayerState::GetSpellCostAfterDiscount(TSubclassOf<USpell> Spell) const
{
	if (Spell)
	{
		const USpell* DefaultSpell = Spell.GetDefaultObject();
		return FMath::Max(0, DefaultSpell->GetSpellStaticCost() - SpellDiscount);
	}

	return -1;
}

bool ACSKPlayerState::CanAffordSpellOfType(ESpellType SpellType) const
{
	for (TSubclassOf<USpellCard> SpellCard : SpellCardsInHand)
	{
		const USpellCard* DefaultSpellCard = SpellCard.GetDefaultObject();
		if (DefaultSpellCard && DefaultSpellCard->CanAffordAnySpell(this, SpellType))
		{
			return true;
		}
	}

	return false;
}

void ACSKPlayerState::GetAffordableSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards, ESpellType SpellType) const
{
	OutSpellCards.Empty();

	for (TSubclassOf<USpellCard> SpellCard : SpellCardsInHand)
	{
		const USpellCard* DefaultSpellCard = SpellCard.GetDefaultObject();
		if (DefaultSpellCard && DefaultSpellCard->CanAffordAnySpell(this, SpellType))
		{
			OutSpellCards.Add(SpellCard);
		}
	}
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
	}
}

void ACSKPlayerState::ResetSpellsCast()
{
	if (HasAuthority())
	{
		SpellsCastThisRound = 0;
	}
}
