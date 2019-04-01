// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerController.h"
#include "BoardTypes.h" // temp
#include "CSKPlayerController.generated.h"

class ACSKPlayerState;
class ATile;

/**
 * Controller for handling communication events between players and the server
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API ACSKPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	// TODO: Manage spells here, since spells should only be known to the player who owns them

public:

	ACSKPlayerController();

public:

	// Begin AActor Interface
	virtual void Tick(float DeltaTime) override;
	// End AActor Interface

protected:

	// Begin APlayerController Interface
	virtual void SetupInputComponent() override;
	// End APlayerController Interface

public:

	/** Get player state as CSK player state */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACSKPlayerState* GetCSKPlayerState() const;

	/** Get the tile currently under this controllers mouse. This only works only local player controllers */
	UFUNCTION(BlueprintCallable, Category = CSK)
	ATile* GetTileUnderMouse() const;

private:

	/** Sets the current tile we are hovering over as selected */
	void SelectTile();

	void AltSelectTile();

protected:

	/** The tile we are currently hovering over (only valid if client owned) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = CSK)
	ATile* HoveredTile;

	UPROPERTY()
	ATile* TestTile1 = nullptr;

	UPROPERTY()
	ATile* TestTile2 = nullptr;

private:

	FBoardPath TestBoardPath;
};
