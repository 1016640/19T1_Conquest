// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerController.h"
#include "CSKGameMode.h"
#include "CSKGameState.h"
#include "CSKHUD.h"
#include "CSKLocalPlayer.h"
#include "CSKPawn.h"
#include "CSKPlayerState.h"
#include "BoardManager.h"
#include "Castle.h"
#include "CastleAIController.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

ACSKPlayerController::ACSKPlayerController()
{
	bShowMouseCursor = true;
	CachedCSKHUD = nullptr;
	CastleController = nullptr;
	CastlePawn = nullptr;
	CSKPlayerID = -1;
	HoveredTile = nullptr;
	bCanSelectTile = false;
	bIsActionPhase = false;
	SelectedAction = ECSKActionPhaseMode::None;
	RemainingActions = ECSKActionPhaseMode::All;
}

void ACSKPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass)
{
	Super::ClientSetHUD_Implementation(NewHUDClass);

	CachedCSKHUD = Cast<ACSKHUD>(MyHUD);
}

void ACSKPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocalPlayerController())
	{
		// Update the tile under the mouse
		ATile* TileUnderMouse = GetTileUnderMouse();
		if (TileUnderMouse)
		{
			// for now
			///HoveredTile = TileUnderMouse;
			// TODO: want to tell tile we are not hovering
			// want to tell new tile we are hovering
		}

		#if WITH_EDITORONLY_DATA
		// Help with debugging while in editor. This change is local to client
		if (HoveredTile)
		{
			HoveredTile->bHighlightTile = false;
		}
		if (TileUnderMouse)
		{
			TileUnderMouse->bHighlightTile = true;
		}
		#endif

		HoveredTile = TileUnderMouse;
	}
}

void ACSKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	
	// Selection
	InputComponent->BindAction("Select", IE_Pressed, this, &ACSKPlayerController::SelectTile);
}

void ACSKPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ACSKPlayerController, CSKPlayerID);
	DOREPLIFETIME(ACSKPlayerController, CastlePawn);
	DOREPLIFETIME(ACSKPlayerController, bIsActionPhase);
	DOREPLIFETIME(ACSKPlayerController, RemainingActions);
}

ACSKPawn* ACSKPlayerController::GetCSKPawn() const
{
	return Cast<ACSKPawn>(GetPawn());
}

ACSKPlayerState* ACSKPlayerController::GetCSKPlayerState() const
{
	return GetPlayerState<ACSKPlayerState>();
}

ACSKHUD* ACSKPlayerController::GetCSKHUD() const
{
	return CachedCSKHUD;
}

ATile* ACSKPlayerController::GetTileUnderMouse() const
{
	// Local player will only exist if we are a client
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (!BoardManager)
		{
			return nullptr;
		}
		
		FVector2D MousePosition;
		if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{
			FVector Position;
			FVector Direction;
			if (UGameplayStatics::DeprojectScreenToWorld(this, MousePosition, Position, Direction))
			{
				FVector End = Position + Direction * 10000.f;
				return BoardManager->TraceBoard(Position, End);
			}
		}
	}

	return nullptr;
}

void ACSKPlayerController::SelectTile()
{
	/*if (!bCanSelectTile)
	{
		return;
	}*/

	// Are we allowed to perform actions
	if (IsPerformingActionPhase())
	{
		switch (SelectedAction)
		{
			case ECSKActionPhaseMode::MoveCastle:
			{
				// TODO: Move this to a function
				if (HoveredTile)
				{
					Server_RequestCastleMoveAction(HoveredTile);
				}
				break;
			}
			case ECSKActionPhaseMode::BuildTowers:
			{
				break;
			}
			case ECSKActionPhaseMode::CastSpell:
			{
				break;
			}
		}
	}
}

void ACSKPlayerController::SetCastleController(ACastleAIController* InController)
{
	if (HasAuthority())
	{
		if (InController && CastleController)
		{
			CastleController->Destroy();
		}

		// Our link to our castle
		CastleController = InController;
		CastlePawn = InController->GetCastle();

		// Link back to us
		CastleController->PlayerOwner = this;

		ACSKPlayerState* PlayerState = GetCSKPlayerState();
		if (PlayerState)
		{
			PlayerState->SetCastle(CastlePawn);
		}
	}
}

void ACSKPlayerController::OnTransitionToBoard()
{
	if (HasAuthority())
	{
		// Occupy the space we are on
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (BoardManager)
		{
			ATile* PortalTile = BoardManager->GetPlayerPortalTile(CSKPlayerID);
			check(PortalTile);

			if (!BoardManager->PlaceBoardPieceOnTile(CastlePawn, PortalTile))
			{
				UE_LOG(LogConquest, Warning, TEXT("Unable to set player %i castle on portal tile as it's already occupied!"), CSKPlayerID);
			}
		}

		// Have client handle any local transition requirements
		Client_OnTransitionToBoard();
	}
}

void ACSKPlayerController::SetActionPhaseEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		if (bIsActionPhase != bEnabled)
		{
			bIsActionPhase = bEnabled;
			RemainingActions = bEnabled ? ECSKActionPhaseMode::All : ECSKActionPhaseMode::None;

			// Purposely calling on rep here to have pawn move back
			if (IsLocalPlayerController())
			{
				OnRep_bIsActionPhase();
			}
			else
			{
				SetActionMode(ECSKActionPhaseMode::None, true);
			}

			// Reset any variables associated with the last round
			if (bEnabled)
			{
				ACSKPlayerState* PlayerState = GetCSKPlayerState();
				if (PlayerState)
				{
					PlayerState->ResetTilesTraversed();
				}
			}
		}
	}
}

void ACSKPlayerController::SetActionMode(ECSKActionPhaseMode NewMode, bool bClientOnly)
{
	if (NewMode == ECSKActionPhaseMode::All)
	{
		UE_LOG(LogConquest, Warning, TEXT("ACSKPlayerController::SetActionMode: PhaseMode::All is not accepted"));
		return;
	}

	if (NewMode != SelectedAction && CanEnterActionMode(NewMode))
	{
		SelectedAction = NewMode;

		// Only call event on the client
		if (IsLocalPlayerController())
		{
			OnSelectionModeChanged(NewMode);
		}

		// We may be setting client side, we should also inform the server
		if (!bClientOnly && !HasAuthority())
		{
			Server_SetActionMode(NewMode);
		}
	}
}

void ACSKPlayerController::BP_SetActionMode(ECSKActionPhaseMode NewMode)
{
	SetActionMode(NewMode);
}

bool ACSKPlayerController::Server_SetActionMode_Validate(ECSKActionPhaseMode NewMode)
{
	return true;
}

void ACSKPlayerController::Server_SetActionMode_Implementation(ECSKActionPhaseMode NewMode)
{
	SetActionMode(NewMode);
}

bool ACSKPlayerController::CanEnterActionMode(ECSKActionPhaseMode ActionMode) const
{
	if (IsPerformingActionPhase())
	{
		return ActionMode == ECSKActionPhaseMode::None ? true : EnumHasAllFlags(RemainingActions, ActionMode);
	}

	return false;
}

bool ACSKPlayerController::CanRequestCastleMoveAction() const
{
	if (IsPerformingActionPhase() && EnumHasAnyFlags(SelectedAction, ECSKActionPhaseMode::MoveCastle))
	{
		// We can only move if we have a castle to move
		return CastlePawn != nullptr && CastleController != nullptr;
	}

	return false;
}

void ACSKPlayerController::OnRep_bIsActionPhase()
{
	SetActionMode(ECSKActionPhaseMode::None, true);

	// Move our camera to our castle when it's our turn to make actions
	if (bIsActionPhase)
	{
		ACSKPawn* Pawn = GetCSKPawn();
		if (Pawn && CastlePawn)
		{
			Pawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
		}
	}
}

void ACSKPlayerController::OnRep_RemainingActions()
{
	if (!EnumHasAllFlags(RemainingActions, SelectedAction))
	{
		SetActionMode(ECSKActionPhaseMode::None, true);
	}
}

void ACSKPlayerController::Client_OnTransitionToBoard_Implementation()
{
	// Move the camera over to our castle (where our portal should be)
	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn)
	{
		if (CastlePawn)
		{
			Pawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
		}
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("Client was unable to travel to castle as pawn was null! (Probaly not replicated yet)"));
	}
}

void ACSKPlayerController::Client_OnCastleMoveRequestConfirmed_Implementation(ACastle* MovingCastle)
{
	bCanSelectTile = false;

	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn)
	{
		Pawn->TrackActor(MovingCastle);
	}
}

void ACSKPlayerController::Client_OnCastleMoveRequestFinished_Implementation()
{
	bCanSelectTile = true;

	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn)
	{
		Pawn->TrackActor(nullptr);
	}
}

void ACSKPlayerController::OnMoveActionFinished()
{
	if (HasAuthority() && IsPerformingActionPhase())
	{
		RemainingActions &= ~ECSKActionPhaseMode::MoveCastle;
	}
}

bool ACSKPlayerController::Server_RequestCastleMoveAction_Validate(ATile* Goal)
{
	return true;
}

void ACSKPlayerController::Server_RequestCastleMoveAction_Implementation(ATile* Goal)
{
	bool bSuccess = false;

	if (CanRequestCastleMoveAction())
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestCastleMove(Goal);
		}		
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}
