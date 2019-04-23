// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKHUD.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"
#include "CSKGameState.h"

#include "BoardManager.h"
#include "UserWidget.h"
#include "Tile.h"
#include "Widgets/CSKHUDWidget.h"

#define LOCTEXT_NAMESPACE "CSKHUD"

ACSKHUD::ACSKHUD()
{
	CSKHUDInstance = nullptr;
}

void ACSKHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UConquestFunctionLibrary::RemoveWidgetFromParent(CSKHUDInstance);
	UConquestFunctionLibrary::RemoveWidgetFromParent(PostMatchWidgetInstance);
}

void ACSKHUD::OnTileHovered(ATile* Tile)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		bool bDisplay = false;

		if (Tile)
		{
			// Prioritize board piece UI data over tile specific
			if (Tile->IsTileOccupied(false))
			{
				FBoardPieceUIData UIData = Tile->GetBoardPieceUIData();
				Widget->SetTileWidgetData(UIData);

				bDisplay = true;
			}
			else if (!Tile->bIsNullTile)
			{
				// This tile has a chance of being a portal tile, we can display this to the user
				ACSKPlayerState* PotentialPortalOwner = GetPlayerStateForPortalTile(Tile);
				if (PotentialPortalOwner)
				{
					FBoardPieceUIData UIData;
					UIData.Owner = PotentialPortalOwner;

					FFormatNamedArguments Args;
					Args.Add("PlayerName", FText::FromString(PotentialPortalOwner->GetPlayerName()));
					UIData.Name = FText::Format(LOCTEXT("PortalUIName", "{PlayerName}s Portal"), Args);

					Widget->SetTileWidgetData(UIData);

					bDisplay = true;
				}
			}
		}

		Widget->ToggleTileWidget(bDisplay);
	}
}

ACSKPlayerState* ACSKHUD::GetPlayerStateForPortalTile(ATile* Tile) const
{
	if (!Tile)
	{
		return nullptr;
	}

	ACSKGameState* CSKGameState = UConquestFunctionLibrary::GetCSKGameState(this);
	if (CSKGameState)
	{
		ABoardManager* BoardManager = CSKGameState->GetBoardManager();

		int32 PortalID = BoardManager ? BoardManager->IsPlayerPortalTile(Tile) : -1;
		if (PortalID != -1)
		{
			return CSKGameState->GetPlayerStateWithID(PortalID);
		}
	}

	return nullptr;
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

void ACSKHUD::OnActionStart(ECSKActionPhaseMode Mode, EActiveSpellContext SpellContext)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnActionStart();

		switch (Mode)
		{
			case ECSKActionPhaseMode::MoveCastle:
			{
				Widget->OnMoveCastleActionStart();
				break;
			}
			case ECSKActionPhaseMode::BuildTowers:
			{
				Widget->OnBuildTowerActionStart();
				break;
			}
			case ECSKActionPhaseMode::CastSpell:
			{
				Widget->OnCastSpellActionStart(SpellContext);
				break;
			}
			default:
			{
				
			}
		}
	}
}

void ACSKHUD::OnActionFinished(ECSKActionPhaseMode Mode, EActiveSpellContext SpellContext)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnActionFinished();

		switch (Mode)
		{
			case ECSKActionPhaseMode::MoveCastle:
			{
				Widget->OnMoveCastleActionFinished();
				break;
			}
			case ECSKActionPhaseMode::BuildTowers:
			{
				Widget->OnBuildTowerActionFinished();
				break;
			}
			case ECSKActionPhaseMode::CastSpell:
			{
				Widget->OnCastSpellActionFinished(SpellContext);
				break;
			}
			default:
			{
				
			}
		}
	}
}

void ACSKHUD::OnQuickEffectSelection(bool bInstigator, const USpell* SpellToCounter, ATile* TargetTile)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnQuickEffectSelection(bInstigator, SpellToCounter, TargetTile);
	}
}

void ACSKHUD::OnBonusSpellSelection(const USpell* BonusSpell)
{
	UCSKHUDWidget* Widget = GetCSKHUDInstance();
	if (Widget)
	{
		Widget->OnBonusSpellSelection(BonusSpell);
	}
}

void ACSKHUD::OnMatchFinished(bool bIsWinner)
{
	UConquestFunctionLibrary::RemoveWidgetFromParent(CSKHUDInstance);

	if (PostMatchWidgetTemplate)
	{
		PostMatchWidgetInstance = CreateWidget<UUserWidget, APlayerController>(PlayerOwner, PostMatchWidgetTemplate);
		UConquestFunctionLibrary::AddWidgetToViewport(PostMatchWidgetInstance);
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

#undef LOCTEXT_NAMESPACE