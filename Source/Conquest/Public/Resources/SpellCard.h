// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Spell.h"
#include "BoardTypes.h"
#include "SpellCard.generated.h"

/**
 * Spell cards contain a list of spells a player can use
 */
UCLASS(abstract, const, Blueprintable, HideCategories = (Object))
class CONQUEST_API USpellCard : public UObject
{
	GENERATED_BODY()

public:

	USpellCard();

public:

	/** Get if this spell card has spells of given type */
	UFUNCTION(BlueprintPure, Category = Spells)
	bool HasSpellOfType(ESpellType SpellType) const;

	/** Check if given player can afford at least one spell of this card */
	UFUNCTION(BlueprintCallable, Category = Spells)
	bool CanAffordAnySpell(const ACSKPlayerState* CastingPlayer, ESpellType SpellType) const;

public:

	/** Get all spells associated with the card */
	FORCEINLINE const TArray<TSubclassOf<USpell>>& GetSpells() const { return Spells; }

	/** Get spell at index */
	FORCEINLINE TSubclassOf<USpell> GetSpellAtIndex(int32 Index) const
	{
		if (Spells.IsValidIndex(Index))
		{
			return Spells[Index];
		}

		// Implicit constructor
		return nullptr;
	}

	/** Get the element types of this spell card */
	FORCEINLINE ECSKElementType GetElementalTypes() const { return static_cast<ECSKElementType>(ElementTypes); }
	
protected:

	/** The name of this spell card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells)
	FName Name;

	/** The spells associated with this spell card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells)
	TArray<TSubclassOf<USpell>> Spells;

	/** The elemental types of this spell card */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (Bitmask, BitmaskEnum = "ECSKElementType"))
	uint8 ElementTypes;
};
