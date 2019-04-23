// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Engine/GameInstance.h"
#include "CSKGameInstance.generated.h"

/**
 * Instance for handling data used throughout CSK
 */
UCLASS()
class CONQUEST_API UCSKGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UCSKGameInstance();

public:

	/** Server travels to given level with options*/
	UFUNCTION(BlueprintCallable, Category = CSK, meta = (WorldContext = "WorldContextObject"))
	static bool ServerTravelToLevel(const UObject* WorldContextObject, FString LevelName, const TArray<FString>& Options);

	// temp
	UFUNCTION(BlueprintImplementableEvent)
	void DestroySessionAfterMatchFinished();
};
