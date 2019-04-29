// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerCameraManager.h"
#include "CSKPlayerController.h"

#include "TimerManager.h"

ACSKPlayerCameraManager::ACSKPlayerCameraManager()
{
	PostFadeOutViewTarget = nullptr;
	PreFadeInDelay = .5f;
}

void ACSKPlayerCameraManager::StopCameraFade()
{
	if (bEnableFading)
	{
		Super::StopCameraFade();

		// Handle the fade sequence is playing
		if (bEnableFadeInFadeOut)
		{
			if (bIsFadingOut)
			{
				//float Delay = FMath::Max(.1f, PreFadeInDelay);

				//// Wait a bit before fading back in
				//FTimerManager& TimerManager = GetWorldTimerManager();
				//TimerManager.SetTimer(Handle_PreFadeIn, this, &ACSKPlayerCameraManager::StartFadeInSection, PreFadeInDelay, false);

				// Instant setting of new view target
				if (PostFadeOutViewTarget)
				{
					PCOwner->SetViewTargetWithBlend(PostFadeOutViewTarget);
				}

				StartFadeInSection();
			}
			else
			{
				ACSKPlayerController* CSKOwner = Cast<ACSKPlayerController>(PCOwner);
				if (CSKOwner)
				{
					CSKOwner->NotifyFadeOutInSequenceFinished();
				}

				bEnableFading = false;
			}
		}
	}
}

void ACSKPlayerCameraManager::StartFadeInAndOutSequence(AActor* InViewTarget, float Duration)
{
	// We might already be executing the sequence
	if (Handle_PreFadeIn.IsValid())
	{
		FTimerManager& TimerManager = GetWorldTimerManager();
		TimerManager.ClearTimer(Handle_PreFadeIn);
	}

	PostFadeOutViewTarget = InViewTarget;
	FadeInOutDuration = Duration;

	bEnableFadeInFadeOut = true;
	bIsFadingOut = true;
	StartCameraFade(0.f, 1.f, FadeInOutDuration, FLinearColor::Black, false, false);
}

void ACSKPlayerCameraManager::StartFadeInSection()
{
	if (bEnableFadeInFadeOut)
	{
		bIsFadingOut = false;
		StartCameraFade(1.f, 0.f, FadeInOutDuration, FLinearColor::Black, false, false);
	}

	Handle_PreFadeIn.Invalidate();
}
