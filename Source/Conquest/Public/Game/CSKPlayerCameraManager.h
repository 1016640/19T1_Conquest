// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "CSKPlayerCameraManager.generated.h"

/**
 * Specialized camera manager for the CSKPlayerController. This handles the transition
 * effects between the coin flip and the board along with other camera operations
 */
UCLASS()
class CONQUEST_API ACSKPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:

	ACSKPlayerCameraManager();

public:

	// Begin APlayerCameraManager Interface
	virtual void StopCameraFade() override;
	// End APlayerCameraManager Interface

public:

	/** Starts the fade-out, fade-in sequence */
	UFUNCTION(BlueprintCallable, Category = CameraManager)
	void StartFadeInAndOutSequence(AActor* InViewTarget, float InDuration);

private:

	/** Initiates the fade in section of the sequence */
	void StartFadeInSection();

public:

	/** The actor to set as next view target when done fading */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = CameraManager)
	AActor* PostFadeOutViewTarget;

	/** How long we should remain in fade out state before fading in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CameraManager)
	float PreFadeInDelay;

private:

	/** If the specialized fade sequence has started */
	uint32 bEnableFadeInFadeOut : 1;

	/** If we are fading in or out */
	uint32 bIsFadingOut : 1;

	/** The duration for both fade in and fade out */
	float FadeInOutDuration;

	/** Timer handle for pre fade in delay */
	FTimerHandle Handle_PreFadeIn;
};
