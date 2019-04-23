// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UUserWidget;

/**
 * Player controller to be used with the main menu game mode
 */
UCLASS()
class CONQUEST_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	AMainMenuPlayerController();

protected:

	// Begin AActor Interface
	virtual void BeginPlay() override;
	// End AActor Interface

public:

	/** Display the current widget while removing the one currently in view */
	UFUNCTION(BlueprintCallable, Category = CSK)
	void SetActiveWidget(TSubclassOf<UUserWidget> WidgetClass);

protected:

	/** The active widget in the viewport */
	UPROPERTY(Transient)
	UUserWidget* ActiveWidget;

private:

	/** The widget to initially display on begin play */
	UPROPERTY(EditDefaultsOnly, Category = CSK)
	TSubclassOf<UUserWidget> DefaultActiveWidget;
};
