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

	SpellActorClass = ASpellActor::StaticClass();
}

int32 USpell::CalculateFinalCost(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile, int32 AdditionalMana) const
{
	return SpellStaticCost;
}

#undef LOCTEXT_NAMESPACE
