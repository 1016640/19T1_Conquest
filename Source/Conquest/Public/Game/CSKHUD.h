// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/HUD.h"
#include "CSKHUD.generated.h"

class UCSKHUDWidget;
class UUserWidget;

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

	/** Notify from our owner that the round state has changed */
	void OnRoundStateChanged(ECSKRoundState NewState);

	/** Notify from our owner that the selection action has changed */
	void OnSelectedActionChanged(ECSKActionPhaseMode NewMode);

	/** Notify that an action or event is starting */
	void OnActionStart();

	/** Notify that an action or event has finished */
	void OnActionFinished();

	/** Notify from our owner that we should refresh the tower list */
	void RefreshTowerList();

private:

	/** Gets the HUD instance, creating it if desired */
	UCSKHUDWidget* GetCSKHUDInstance(bool bCreateIfNull = true);

protected:

	/** Instance of the CSKHUD widget */
	UPROPERTY()
	UCSKHUDWidget* CSKHUDInstance;

protected:
	
	/** The widget to use when entering game loop */
	UPROPERTY(EditAnywhere, Category = CSK)
	TSubclassOf<UCSKHUDWidget> CSKHUDTemplate;
};
