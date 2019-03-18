// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CSKPlayerController.generated.h"

/**
 * Controller for handling communication events between players and the server
 */
UCLASS()
class CONQUEST_API ACSKPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	// TODO: Manage spells here, since spells should only be known to the player who owns them
};
