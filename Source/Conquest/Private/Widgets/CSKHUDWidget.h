// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CSKHUDWidget.generated.h"

enum class ECSKRoundState : uint8;
enum class ECSKActionPhaseMode : uint8;

/**
 * Specialized HUD for the CSKHUD to interact with
 */
UCLASS()
class UCSKHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
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

	/** Notify that we should refresh the tower list */
	UFUNCTION(BlueprintImplementableEvent)
	void RefreshTowerList();
};
