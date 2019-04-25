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

bool USpellCard::CanAffordAnySpell(const ACSKPlayerState* CastingPlayer, ESpellType SpellType, bool bNullifySpells) const
{
	if (!CastingPlayer)
	{
		return false;
	}

	for (const TSubclassOf<USpell>& Spell : Spells)
	{
		const USpell* DefaultSpell = Spell.GetDefaultObject();
		if (!DefaultSpell)
		{
			UE_LOG(LogConquest, Warning, TEXT("USpellCard: Invalid spell found in %s"), *GetName());
			continue;
		}

		if (DefaultSpell->GetSpellType() == SpellType)
		{
			// If querying quick effect spells, we either want nullify or post spells
			if (DefaultSpell->GetSpellType() == ESpellType::QuickEffect &&
				DefaultSpell->NullifiesOtherSpell() != bNullifySpells)
			{
				continue;
			}

			// We only compare static cost, we don't have the
			// information to calculate a dynamic cost at this time
			if (DefaultSpell->ExpectsAdditionalMana())
			{
				if (CastingPlayer->HasRequiredManaPlusAdditionalAmount(DefaultSpell->GetSpellStaticCost()))
				{
					return true;
				}
			}
			else
			{
				if (CastingPlayer->HasRequiredMana(DefaultSpell->GetSpellStaticCost(), true))
				{
					return true;
				}
			}
		}
	}

	return false;
}
