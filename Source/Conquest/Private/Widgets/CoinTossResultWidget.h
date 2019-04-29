// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoinTossResultWidget.generated.h"

/**
 * Displays the winner of the coin toss. Simply passes if
 * the owners player is the player who won the coin toss
 */
UCLASS(abstract)
class UCoinTossResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Display the coin information. Passes if our owner won the toss */
	UFUNCTION(BlueprintImplementableEvent)
	void DisplayWinner(bool bLocalPlayerWon);
};
