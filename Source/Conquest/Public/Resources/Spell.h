// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BoardTypes.h"
#include "SpellEffect.h"
#include "Spell.generated.h"

/** Type used to indentify when a spell can be used */
UENUM(BlueprintType)
enum class ESubSpellType : uint8
{
	/** This spell is used during the users action phase */
	ActionPhase,

	/** This spell can be used to counter an opponents spell */
	QuickEffect
};

/**
 * A sub spell for any spell
 */
USTRUCT()
struct CONQUEST_API FSubSpell
{
	GENERATED_BODY()

public:

	FSubSpell()
	{

	}

public:

	// In mana
	int32 Cost;

	// Type of spell
	ESubSpellType Type;

	// Effects this sub spell applies
	TArray<TSubclassOf<USpellEffect>> Effects;
};

/**
 * Spells that players can use to attack or heal tiles on the map
 */
UCLASS(Const, Abstract, Blueprintable, ClassGroup = "Resources")
class CONQUEST_API USpell : public UObject
{
	GENERATED_BODY()
	
public:

	USpell();

protected:

	/** */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Spell, meta = (DisplayName="Spell"))
	FName SpellName;

	/** */
	TArray<FSubSpell> SubSpells;
};
