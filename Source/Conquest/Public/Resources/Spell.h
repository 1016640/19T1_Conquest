// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "Spell.generated.h"

class ACSKPlayerState;
class ASpellActor;
class ATile;
class USpellWidget;

/** Type used to indentify when a spell can be used */
UENUM(BlueprintType)
enum class ESpellType : uint8
{
	/** This spell is used during the users action phase */
	ActionPhase,

	/** This spell can be used to counter an opponents spell */
	QuickEffect,

	/** This spell is a bonus spell for using a spell card that has
	the same element as the tile the players castle is currently on.
	These spells can not be cast by players and only manually by the game, thus they have no cost */
	ElementBonus
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
	target is required, the target to use will default to the players castle.
	This only applies to spells that are marked as being bonus spells */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Spells)
	bool RequiresTarget() const;

	/** Check if spell can be used on given tile. This can get called on clients,
	but will ultimately be called on the server to validate before allowing cast */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Spells)
	bool CanActivateSpell(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile) const;

	/** Calculates the final cost of this spell. Passes in the player casting the spell, the
	tile they plan to cast it onto, the discounted static cost and how much additional mana 
	they are willing to spend. By default, will return DiscountedCost + AdditionalMana */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = Spells)
	int32 CalculateFinalCost(const ACSKPlayerState* CastingPlayer, const ATile* TargetTile, int32 DiscountedCost, int32 AdditionalMana) const;

public:

	/** Get this spells name */
	FORCEINLINE const FName& GetSpellName() const { return SpellName; }

	/** Get this spells description */
	FORCEINLINE const FText& GetSpellDescription() const { return SpellDescription; }

	/** Get this spells type */
	FORCEINLINE ESpellType GetSpellType() const { return SpellType; }

	/** Get this spells static cost */
	FORCEINLINE int32 GetSpellStaticCost() const { return SpellStaticCost; }

	/** Get this spells override widget */
	FORCEINLINE TSubclassOf<USpellWidget> GetSpellWidgetOverride() const { return SpellWidgetOverride; }

	/** Get this spells actor class */
	FORCEINLINE TSubclassOf<ASpellActor> GetSpellActorClass() const { return SpellActorClass; }

	/** Get if this spell expects additional mana */
	FORCEINLINE bool ExpectsAdditionalMana() const { return bSpellExpectsAdditionalMana; }

	/** Get if this spell nullifies other spells (only valid for quick effects ) */
	FORCEINLINE bool NullifiesOtherSpell() const { return bSpellNullifiesSpells; }

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

	/** The widget to use instead of a default details item widget. This should be
	used for any spells that require additional data be passed (e.g. additional mana) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells)
	TSubclassOf<USpellWidget> SpellWidgetOverride;

	/** The spell actor to spawn. This actor handles casting the spell */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Actor Class"))
	TSubclassOf<ASpellActor> SpellActorClass;

	/** If this spell needs a target in order to activate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spells, meta = (DisplayName = "Requires Target"))
	uint8 bSpellRequiresTarget : 1;

	/** If this spell expects additional mana. This will simply alter the default behavior of CalculateFinalCost
	with true returning DiscountedCost + AdditionalMana while false simply returns DiscountedCost */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay, Category = Spells, meta = (DisplayName="Expects Additional Mana"))
	uint8 bSpellExpectsAdditionalMana : 1;

	/** If this spell is a quick effect, do we instantly nullify the opponents spell or activate afterwards */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay, Category = Spells, meta = (DisplayName = "Nullifies Other Spells"))
	uint8 bSpellNullifiesSpells : 1;
};
