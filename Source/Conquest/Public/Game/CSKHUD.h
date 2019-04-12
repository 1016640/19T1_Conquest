// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/HUD.h"
#include "CSKHUD.generated.h"

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

private:

	/** Get the widget associated with the given round state */
	TSubclassOf<UUserWidget> GetWidgetAssociatedWithState(ECSKRoundState RoundState) const;

	/** Removes current widget from viewport and replaces it with given one */
	void ReplaceWidgetInViewport(TSubclassOf<UUserWidget> NewWidget);

protected:

	/** The widget currently in the viewport */
	UPROPERTY()
	UUserWidget* WidgetInViewport;

protected:
	
	/** The HUD widget to display during collection phase (Can be Null) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CSK)
	TSubclassOf<UUserWidget> CollectionPhaseWidget;

	/** The HUD widget to display when it's our owners action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CSK)
	TSubclassOf<UUserWidget> PlayingActionPhaseWidget;

	/** The HUD widget to display when it's the opposing players action phase */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CSK)
	TSubclassOf<UUserWidget> WaitingActionPhaseWidget;

	/** The HUD widget to display during end round phase (Can be Null) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CSK)
	TSubclassOf<UUserWidget> EndRoundPhaseWidget;
};
