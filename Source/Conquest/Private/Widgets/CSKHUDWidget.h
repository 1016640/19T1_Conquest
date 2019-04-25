// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CSKHUDWidget.generated.h"

class USpell;
enum class ECSKRoundState : uint8;
enum class ECSKActionPhaseMode : uint8;
struct FBoardPieceUIData;

/**
 * Specialized HUD for the CSKHUD to interact with
 */
UCLASS()
class UCSKHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Notify if we should display or hide the tiles information */
	UFUNCTION(BlueprintImplementableEvent)
	void ToggleTileWidget(bool bDisplay);

	/** Notify that player has hovered over a new tile with a board piece on it */
	UFUNCTION(BlueprintImplementableEvent)
	void SetTileWidgetData(const FBoardPieceUIData& UIData);

public:

	/** Notify that the round state has changed */
	UFUNCTION(BlueprintImplementableEvent)
	void OnRoundStateChanged(ECSKRoundState NewState, bool bIsPlayersTurn);
	
	/** Notify that the player has selected the given action */
	UFUNCTION(BlueprintImplementableEvent)
	void OnSelectedActionChanged(ECSKActionPhaseMode NewMode);

	/** Notify that an action (event) is starting */
	UFUNCTION(BlueprintImplementableEvent)
	void OnActionStart();

	/** Notify that an action (event) has finished */
	UFUNCTION(BlueprintImplementableEvent)
	void OnActionFinished();

	/** Notify that a castle move action has started */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMoveCastleActionStart();

	/** Notify that a castle move action has finished */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMoveCastleActionFinished();

	/** Notify that a build tower action has started */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBuildTowerActionStart();

	/** Notify that a build tower action has finished */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBuildTowerActionFinished();

	/** Notify that a cast spell action has started */
	UFUNCTION(BlueprintImplementableEvent)
	void OnCastSpellActionStart(EActiveSpellContext Context);

	/** Notify that a cast spell action has finished */
	UFUNCTION(BlueprintImplementableEvent)
	void OnCastSpellActionFinished(EActiveSpellContext Context);

	/** Notify that a player has started to select a spell to counter a pending request.
	This gets called for either our owner selecting or our owner waiting */
	UFUNCTION(BlueprintImplementableEvent)
	void OnQuickEffectSelection(bool bIsSelecting, bool bNullify, const USpell* SpellToCounter, ATile* TargetTile);

	/** Notify that the player is allowed to select a target for bonus spell */
	UFUNCTION(BlueprintImplementableEvent)
	void OnBonusSpellSelection(bool bIsSelecting, const USpell* BonusSpell);
};
