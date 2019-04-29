// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpellWidget.generated.h"

class USpell;

/** Delegate for when spell card has been selected */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSpellSelected, TSubclassOf<USpell>, Spell, int32, SpellIndex, int32, AdditionalMana);

/** Delegate for when additional mana has been changed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAdditionalManaChanged, int32, AdditionalMana);

/**
 * Base for widgets that represent a spell card
 */
UCLASS()
class USpellWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Event that is called when a spell has been selected */
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FSpellSelected OnSpellSelected;

	/** Event that is called when additional mana has been changed */
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FAdditionalManaChanged OnAdditionalManaChanged;

private:

	/** The spell associated with this widget */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = "true", AllowPrivateAccess = "true"))
	TSubclassOf<USpell> Spell;

	/** The index of this spell in the spell card */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = "true", AllowPrivateAccess = "true"))
	int32 SpellIndex;

	/** If this particular spell should be selectable (This is required
	so this spell still appears along side other spells on a spell card */
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = "true", AllowPrivateAccess = "true"))
	uint8 bCanSelectSpell : 1;
};
