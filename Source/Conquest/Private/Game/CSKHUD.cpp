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
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		UConquestFunctionLibrary::AddWidgetToViewport(Widget);

		bool bIsOurTurn = false;

		// Determine if it's our owners turn
		if (NewState == ECSKRoundState::FirstActionPhase || NewState == ECSKRoundState::SecondActionPhase)
		{
			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState)
			{
				ACSKPlayerController* Controller = CastChecked<ACSKPlayerController>(PlayerOwner);
				bIsOurTurn = GameState->GetActionPhasePlayerID() == Controller->CSKPlayerID;
			}
		}

		Widget->OnRoundStateChanged(NewState, bIsOurTurn);
	}
}

void ACSKHUD::OnSelectedActionChanged(ECSKActionPhaseMode NewMode)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnSelectedActionChanged(NewMode);
	}
}

void ACSKHUD::OnActionStart()
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnActionStart();
	}
}

void ACSKHUD::OnActionFinished()
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnActionFinished();
	}
}

UCSKHUDWidget* ACSKHUD::GetCSKHUDInstance(bool bCreateIfNull)
{
	if (!CSKHUDInstance && bCreateIfNull)
	{
		if (!CSKHUDTemplate)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::GetCSKHUDInstance: HUD template is null"));
			return nullptr;
		}

		CSKHUDInstance = CreateWidget<UCSKHUDWidget, APlayerController>(PlayerOwner, CSKHUDTemplate);
		if (!CSKHUDInstance)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKHUD::GetCSKHUDInstance: Failed to create Widget"));
		}
	}

	return CSKHUDInstance;
}
