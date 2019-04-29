// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/HUD.h"
#include "CSKHUD.generated.h"

class ACSKPlayerState;
class ATile;
class UCoinTossResultWidget;
class UCSKHUDWidget;
class UUserWidget;
class USpell;

/**
 * Manages the widgets and details displayed on screen during CSK
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKHUD : public AHUD
{
	GENERATED_BODY()

public:

	ACSKHUD();

public:

	// Begin AActor Interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End AActor Interface
	
public:

	/** Notify that player has hovered over a new tile */
	void OnTileHovered(ATile* Tile);

private:

	/** Helper function for getting the player state corresponding with a portal tile */
	ACSKPlayerState* GetPlayerStateForPortalTile(ATile* Tile) const;

public:

	/** Notify from our owner that the round state has changed */
	void OnRoundStateChanged(ECSKRoundState NewState);

	/** Notify from our owner that the selection action has changed */
	void OnSelectedActionChanged(ECSKActionPhaseMode NewMode);

	/** Notify that an action or event is starting */
	void OnActionStart(ECSKActionPhaseMode Mode, EActiveSpellContext SpellContext);

	/** Notify that an action or event has finished */
	void OnActionFinished(ECSKActionPhaseMode Mode, EActiveSpellContext SpellContext);

	/** Notify that the opposing player (to whose action phase it is) has started to select a counter spell.
	Passes in if our owner is the one instigating this selection (the one who can counter the spell) */
	void OnQuickEffectSelection(bool bInstigator, bool bNullify, const USpell* SpellToCounter, ATile* TargetTile);

	/** Notify that this player is able to select a tile to use the given bonus spell on */
	void OnBonusSpellSelection(bool bInstigator, const USpell* BonusSpell);

public:

	/** Notify to toggle the coin toss result widget */
	void OnToggleCoinTossResult(bool bDisplay, bool bIsWinner);

	/** Notify that the match has finished and the post match widget should be display */
	void OnMatchFinished(bool bIsWinner);

private:

	/** Gets the HUD instance, creating it if desired */
	UCSKHUDWidget* GetCSKHUDInstance(bool bCreateIfNull = true);

protected:

	/** Instance of the coin toss result widget */
	UPROPERTY()
	UCoinTossResultWidget* CoinTossWidgetInstance;

	/** Instance of the CSKHUD widget */
	UPROPERTY()
	UCSKHUDWidget* CSKHUDInstance;

	/** Instance of the Post Match widget */
	UPROPERTY()
	UUserWidget* PostMatchWidgetInstance;

protected:

	/** The widget to use to display the coin toss result */
	UPROPERTY(EditAnywhere, Category = CSK)
	TSubclassOf<UCoinTossResultWidget> CoinTossWidgetTemplate;
	
	/** The widget to use when entering game loop */
	UPROPERTY(EditAnywhere, Category = CSK)
	TSubclassOf<UCSKHUDWidget> CSKHUDTemplate;

	/** The widget to use during the post match phase */
	UPROPERTY(EditAnywhere, Category = CSK)
	TSubclassOf<UUserWidget> PostMatchWidgetTemplate;
};
