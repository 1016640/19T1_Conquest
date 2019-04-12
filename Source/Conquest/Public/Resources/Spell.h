// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "Spell.generated.h"

class ACSKPlayerState;
class ASpellActor;
class ATile;

/** Type used to indentify when a spell can be used */
UENUM(BlueprintType)
enum class ESpellType : uint8
{
	/** This spell is used during the users action phase */
	ActionPhase,

	/** This spell can be used to counter an opponents spell */
	QuickEffect
};

/**
 * An indiviual spell a player can use. Spells are attached to a spell card and are used
 * to verify costs and ultimately spawn in the spell actor which handles casting the spell.
 */
UCLASS(const, abstract, Blueprintable, HideCategories = (Object))
class CONQUEST_API USpell : public UObject
{
	GENERATED_BODY()
	
public:

	USpell();

public:

	/** If this spell requires a target tile in order to activate. If no
	target is required, the target to use will default to the players castle */
	UFUNCTION(BlueprintPure, Category = Spells)
	virtual bool RequiresTarget() const { return bSpellRequiresTarget; }

	/** Check if spell can be used on given tile */
	UFUNCTION(BlueprintPure, Category = Spells)
	virtual bool CanActivateSpell(const ATile* TargetTile) const { return true; }

	/** Calculates the final cost of this spell. Passes in the player casting the spell, the
	tile they plan to cast it onto and how much additional mana they are willing to spend */
	UFUNCTION(BlueprintPure, Category = Spells)
	virtual int32 CalculateFinalCost(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile, int32 AdditionalMana) const;

public:

	/** Get this spells name */
	FORCEINLINE const FName& GetSpellName() const { return SpellName; }

	/** Get this spells description */
	FORCEINLINE const FText& GetSpellDescription() const { return SpellDescription; }

	/** Get this spells type */
	FORCEINLINE ESpellType GetSpellType() const { return SpellType; }

	/** Get this spells static cost */
	FORCEINLINE int32 GetSpellStaticCost() const { return SpellStaticCost; }

	/** Get this spells actor class */
	FORCEINLINE TSubclassOf<ASpellActor> GetSpellActorClass() const { return SpellActorClass; }

protected:

	/** The name of this spell */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName="Name"))
	FName SpellName;

	/** A description of what this spell does */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Description", MultiLine = "true"))
	FText SpellDescription;

	/** The type of this spell */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Type"))
	ESpellType SpellType;

	/** The static cost of this spell. This amount of mana is required in order
	to perform the spell, but the final cost can be calculated at run time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Cost"))
	int32 SpellStaticCost;

	// TODO: Widget to use instead of default one (this is required as some spells are
	// dynamic based on how much mana can be set to use it)

	/** The spell actor to spawn. This actor handles casting the spell */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Actor Class"))
	TSubclassOf<ASpellActor> SpellActorClass;

	/** If this spell needs a target in order to activate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Requires Target"))
	uint8 bSpellRequiresTarget : 1;
};
