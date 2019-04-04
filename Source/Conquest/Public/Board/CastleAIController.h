// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "AIController.h"
#include "CastleAIController.generated.h"

class ACastle;
class ACSKPlayerController;
class UBoardPathFollowingComponent;
struct FBoardPath;

/**
 * AI controller that will handle movement for the castle pawn.
 * Each player will have their own castle controller to manage
 */
UCLASS()
class CONQUEST_API ACastleAIController : public AAIController
{
	GENERATED_BODY()
	
public:

	ACastleAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	/** The player that owns this castle */
	UPROPERTY(Transient, DuplicateTransient)
	ACSKPlayerController* PlayerOwner;

public:

	/** Will follow the given path (only if not already following one) */
	void FollowPath(const FBoardPath& InPath);

private:

	/** Notify that we have finished a segment of the current path */
	UFUNCTION()
	void OnBoardPathSegmentCompleted(ATile* SegmentTile);

	/** Notify that we have finished following current path */
	UFUNCTION()
	void OnBoardPathCompleted(ATile* DestinationTile);

public:

	/** Get possessed pawn as a castle */
	UFUNCTION(BlueprintPure, Category = CSK)
	ACastle* GetCastle() const;

	/** Get path following component as a board path following component */
	UFUNCTION(BlueprintPure, Category = CSK)
	UBoardPathFollowingComponent* GetBoardPathFollowingComponent() const;
};
