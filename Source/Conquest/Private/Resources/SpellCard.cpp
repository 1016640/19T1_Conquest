// Fill out your copyright notice in the Description page of Project Settings.

#include "SpellCard.h"
#include "CSKPlayerState.h"

USpellCard::USpellCard()
{
	Name = TEXT("Spell Card");
}

bool USpellCard::HasSpellOfType(ESpellType SpellType) const
{
	for (const TSubclassOf<USpell>& Spell : Spells)
	{
		const USpell* DefaultSpell = Spell.GetDefaultObject();
		check(DefaultSpell);

		if (DefaultSpell->GetSpellType() == SpellType)
		{
			return true;
		}
	}
	
	return false;
}

bool USpellCard::CanAffordAnySpell(const ACSKPlayerState* CastingPlayer, ESpellType SpellType) const
{
	if (!CastingPlayer)
	{
		return false;
	}

	for (const TSubclassOf<USpell>& Spell : Spells)
	{
		const USpell* DefaultSpell = Spell.GetDefaultObject();
		check(DefaultSpell);

		if (DefaultSpell->GetSpellType() == SpellType)
		{
			// We only compare static cost, we don't have the
			// information to calculate a dynamic cost at this time
			if (CastingPlayer->HasRequiredMana(DefaultSpell->GetSpellStaticCost()))
			{
				return true;
			}
		}
	}

	return false;
}
