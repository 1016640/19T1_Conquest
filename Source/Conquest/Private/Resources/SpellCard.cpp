// Fill out your copyright notice in the Description page of Project Settings.

#include "SpellCard.h"

bool USpellCard::HasSpellOfType(ESpellType SpellType) const
{
	for (const TSubclassOf<USpell>& Spell : Spells)
	{
		if (Spell)
		{
			USpell* DefaultSpell = Spell.GetDefaultObject();
			check(DefaultSpell);

			if (DefaultSpell->GetSpellType() == SpellType)
			{
				return true;
			}
		}
	}
	
	return false;
}