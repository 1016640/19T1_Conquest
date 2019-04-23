// Fill out your copyright notice in the Description page of Project Settings.

#include "Spell.h"
#include "SpellActor.h"

#define LOCTEXT_NAMESPACE "USpell"

USpell::USpell()
{
	SpellName = TEXT("Spell");
	SpellDescription = LOCTEXT("SpellDescription", "Casts a spell");
	SpellType = ESpellType::ActionPhase;
	SpellStaticCost = 5;
	bSpellRequiresTarget = false;
	bSpellExpectsAdditionalMana = false;
	bSpellNullifiesSpells = false;

	SpellActorClass = ASpellActor::StaticClass();
}

bool USpell::RequiresTarget_Implementation() const 
{ 
	return bSpellRequiresTarget; 
}

bool USpell::CanActivateSpell_Implementation(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile) const
{
	return TargetTile != nullptr;
}

int32 USpell::CalculateFinalCost_Implementation(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile, int32 DiscountedCost, int32 AdditionalMana) const
{
	if (bSpellExpectsAdditionalMana)
	{
		return DiscountedCost + AdditionalMana;
	}
	else
	{
		return DiscountedCost;
	}
}

#undef LOCTEXT_NAMESPACE
