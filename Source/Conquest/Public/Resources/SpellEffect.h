// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "UObject/NoExportTypes.h"
#include "SpellEffect.generated.h"

class ACSKPlayerState;
class ATile;

/**
 * An effect a spell will use when activated
 */
UCLASS(Const, Abstract, Blueprintable, ClassGroup="Resources")
class CONQUEST_API USpellEffect : public UObject
{
	GENERATED_BODY()

public:

	/** Event for applying the contents of this effect */
	UFUNCTION(BlueprintNativeEvent, Category = Spell, meta = (DisplayName = "Apply Effect"))
	void ApplySpellEffect(ACSKPlayerState* Player, ATile* TargetTile, int32 Cost) const;
	virtual void ApplySpellEffect_Implementation(ACSKPlayerState* Player, ATile* TargetTile, int32 Cost) const PURE_VIRTUAL(ApplySpellEffect_Implementation, );

	// TODO: Need to have effects check if target tile is valid
};

/**
 * Spell effect that applies healing 
 */
UCLASS()
class CONQUEST_API USpellEffect_Healing : public USpellEffect
{
	GENERATED_BODY()

public:

	// Begin USpellEffect Interface
	virtual void ApplySpellEffect_Implementation(ACSKPlayerState* Player, ATile* TargetTile, int32 Cost) const override { }
	// End USpellEffect Interface
};
