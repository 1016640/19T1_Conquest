// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKHUD.h"
#include "CSKPlayerController.h"
#include "CSKGameState.h"

#include "UserWidget.h"

ACSKHUD::ACSKHUD()
{
	WidgetInViewport = nullptr;
}

void ACSKHUD::OnRoundStateChanged(ECSKRoundState NewState)
{
	ReplaceWidgetInViewport(GetWidgetAssociatedWithState(NewState));
}

TSubclassOf<UUserWidget> ACSKHUD::GetWidgetAssociatedWithState(ECSKRoundState RoundState) const
{
	switch (RoundState)
	{
		case ECSKRoundState::CollectionPhase:
		{
			return CollectionPhaseWidget;
		}
		case ECSKRoundState::EndRoundPhase:
		{
			return EndRoundPhaseWidget;
		}
		// Intentional fallthrough
		case ECSKRoundState::FirstActionPhase:
		case ECSKRoundState::SecondActionPhase:
		{
			ACSKPlayerController* CSKController = CastChecked<ACSKPlayerController>(PlayerOwner);

			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState)
			{
				if (GameState->GetActionPhasePlayerID() == CSKController->CSKPlayerID)
				{
					return PlayingActionPhaseWidget;
				}
				else
				{
					return WaitingActionPhaseWidget;
				}
			}
			else
			{
				UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::GetWidgetAssociatedWithState: Game state is not of ACSKGameState."));
				return nullptr;
			}
		}
	}

	return nullptr;
}

void ACSKHUD::ReplaceWidgetInViewport(TSubclassOf<UUserWidget> NewWidget)
{
	// This function handles null checks
	UConquestFunctionLibrary::RemoveWidgetFromParent(WidgetInViewport);
	WidgetInViewport = nullptr;

	if (NewWidget)
	{
		WidgetInViewport = CreateWidget<UUserWidget, APlayerController>(PlayerOwner, NewWidget);
		if (!WidgetInViewport)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::ReplaceWidgetInViewport: Failed to create Widget"));
		}
		
		// This function handles null checks
		UConquestFunctionLibrary::AddWidgetToViewport(WidgetInViewport);
	}
}
