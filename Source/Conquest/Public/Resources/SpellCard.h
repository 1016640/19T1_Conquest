// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Spell.h"
#include "SpellCard.generated.h"

/**
 * Spell cards contain a list of spells a player can use
 */
UCLASS(abstract, const, Blueprintable, HideCategories = (Object))
class CONQUEST_API USpellCard : public UObject
{
	GENERATED_BODY()

public:

	/** Get if this spell card has spells of given type */
	UFUNCTION(BlueprintPure, Category = Spells)
	bool HasSpellOfType(ESpellType SpellType) const;

public:

	/** Get all spells associated with the card */
	FORCEINLINE const TArray<TSubclassOf<USpell>>& GetSpells() const { return Spells; }
	
protected:

	/** The spells associated with this spell card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spell)
	TArray<TSubclassOf<USpell>> Spells;
};
