// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKHUD.h"
#include "CSKPlayerController.h"
#include "CSKGameState.h"

#include "UserWidget.h"
#include "Widgets/CSKHUDWidget.h"

ACSKHUD::ACSKHUD()
{
	CSKHUDInstance = nullptr;
}

void ACSKHUD::OnRoundStateChanged(ECSKRoundState NewState)
{
	if (!CSKHUDInstance)
	{
		if (!CSKHUDTemplate)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::OnRoundStateChanged: HUD template is null"));
			return;
		}

		CSKHUDInstance = CreateWidget<UCSKHUDWidget, APlayerController>(PlayerOwner, CSKHUDTemplate);
		if (!CSKHUDInstance)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::OnRoundStateChanged: Failed to create Widget"));
		}

		// Handles null checks
		UConquestFunctionLibrary::AddWidgetToViewport(CSKHUDInstance);
	}

	if (CSKHUDInstance)
	{
		bool bIsOwnersTurn = false;

		// Determine if it's our owners turn
		if (NewState == ECSKRoundState::FirstActionPhase || NewState == ECSKRoundState::SecondActionPhase)
		{
			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState)
			{
				ACSKPlayerController* Controller = CastChecked<ACSKPlayerController>(PlayerOwner);
				bIsOwnersTurn = GameState->GetActionPhasePlayerID() == Controller->CSKPlayerID;
			}
		}

		CSKHUDInstance->OnRoundStateChanged(NewState, bIsOwnersTurn);
	}
}

void ACSKHUD::OnActionStart()
{
	if (CSKHUDInstance)
	{
		CSKHUDInstance->OnActionStart();
	}
}

void ACSKHUD::OnActionFinished()
{
	if (CSKHUDInstance)
	{
		CSKHUDInstance->OnActionFinished();
	}
}

void ACSKHUD::RefreshTowerList()
{
	if (CSKHUDInstance)
	{
		CSKHUDInstance->RefreshTowerList();
	}
}
