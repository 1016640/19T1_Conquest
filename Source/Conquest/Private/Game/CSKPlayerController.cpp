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
#include "Tower.h"

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
	bWaitingOnTallyEvent = false;
	bIsActionPhase = false;
	SelectedAction = ECSKActionPhaseMode::None;
	RemainingActions = ECSKActionPhaseMode::All;
}

void ACSKPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass)
{
	Super::ClientSetHUD_Implementation(NewHUDClass);

	CachedCSKHUD = Cast<ACSKHUD>(MyHUD);
}

void ACSKPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (GameState)
		{
			GameState->OnRoundStateChanged.AddDynamic(this, &ACSKPlayerController::OnRoundStateChanged);
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKPlayerController: Unable to bind round state "
				"change event as game state is not of CSKGameState"));
		}
	}
}

void ACSKPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalPlayerController())
	{
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (GameState)
		{
			GameState->OnRoundStateChanged.RemoveDynamic(this, &ACSKPlayerController::OnRoundStateChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ACSKPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocalPlayerController())
	{
		// Get tile player is hovering over
		ATile* TileUnderMouse = GetTileUnderMouse();
		if (TileUnderMouse != HoveredTile)
		{
			if (HoveredTile)
			{
				HoveredTile->EndHoveringTile(this);
			}

			HoveredTile = TileUnderMouse;

			if (HoveredTile)
			{
				HoveredTile->StartHoveringTile(this);
			}
		}
	}
}

void ACSKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	
	// Selection
	InputComponent->BindAction("Select", IE_Pressed, this, &ACSKPlayerController::SelectTile);

	// Shortcuts
	InputComponent->BindAction("ResetCamera", IE_Pressed, this, &ACSKPlayerController::ResetCamera);
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
	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (!BoardManager)
	{
		return nullptr;
	}

	FVector Location;
	FVector Direction;
	if (DeprojectMousePositionToWorld(Location, Direction))
	{
		FVector End = Location + Direction * 10000.f;
		return BoardManager->TraceBoard(Location, End);
	}

	return nullptr;
}

void ACSKPlayerController::SelectTile()
{
	if (false)//TODO: !bCanSelectTile)
	{
		return;
	}

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
				// TODO: Move this to a function
				if (HoveredTile)
				{
					Server_RequestBuildTowerAction(TestTowerTemplate, HoveredTile);
				}
				break;
			}
			case ECSKActionPhaseMode::CastSpell:
			{
				// TODO: Move this to a function
				if (HoveredTile)
				{
					Server_RequestCastSpellAction(TestSpellCardTemplate, TestSpellCardSpellIndex, HoveredTile, 0);
				}
				break;
			}
		}
	}
}

void ACSKPlayerController::ResetCamera()
{
	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn && CastlePawn)
	{
		Pawn->TravelToLocation(CastlePawn->GetActorLocation());
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

void ACSKPlayerController::OnRoundStateChanged(ECSKRoundState NewState)
{
	switch (NewState)
	{
		case ECSKRoundState::CollectionPhase:
		{
			// Ignore value could stack when going from end round phase to collection phase
			if (!IsMoveInputIgnored())
			{
				SetIgnoreMoveInput(true);
			}

			ACSKPawn* Pawn = GetCSKPawn();
			if (Pawn && CastlePawn)
			{
				// Focus on our castle while we wait for tallied resources
				Pawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
			}

			bWaitingOnTallyEvent = false;

			break;
		}
		case ECSKRoundState::FirstActionPhase:
		{
			ResetIgnoreMoveInput();

			// Tally event should have concluded by now
			if (bWaitingOnTallyEvent)
			{
				UE_LOG(LogConquest, Warning, TEXT("ACSKPlayerController::OnCollectionResourcesTallied: Event did not call FinishCollectionTallyEvent()."));
				bWaitingOnTallyEvent = false;
			}

			break;
		}
		case ECSKRoundState::SecondActionPhase:
		{
			ResetIgnoreMoveInput();

			break;
		}
		case ECSKRoundState::EndRoundPhase:
		{
			SetIgnoreMoveInput(true);

			break;
		}
	}

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnRoundStateChanged(NewState);
	}
}

void ACSKPlayerController::OnTransitionToBoard()
{
	if (HasAuthority())
	{
		// TODO: Move this to game mode (game mode should handle place board pieces)
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

void ACSKPlayerController::Client_OnCollectionPhaseResourcesTallied_Implementation(FCollectionPhaseResourcesTally TalliedResources)
{
	bWaitingOnTallyEvent = true;
	OnCollectionResourcesTallied(TalliedResources.Gold, TalliedResources.Mana, TalliedResources.bDeckReshuffled, TalliedResources.SpellCard);
}

void ACSKPlayerController::OnCollectionResourcesTallied_Implementation(int32 Gold, int32 Mana, bool bDeckReshuffled, TSubclassOf<USpellCard> SpellCard)
{
	// Just end immediately if not implemented
	FinishCollectionSequenceEvent();
}

void ACSKPlayerController::FinishCollectionSequenceEvent()
{
	if (bWaitingOnTallyEvent)
	{
		Server_FinishCollecionSequence();
		bWaitingOnTallyEvent = false;
	}
}

bool ACSKPlayerController::Server_FinishCollecionSequence_Validate()
{
	return true;
}

void ACSKPlayerController::Server_FinishCollecionSequence_Implementation()
{
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		GameMode->NotifyCollectionPhaseSequenceFinished(this);
	}
}

void ACSKPlayerController::SetActionPhaseEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		if (bIsActionPhase != bEnabled)
		{
			bIsActionPhase = bEnabled;

			// Purposely calling on rep here to have pawn move back
			if (IsLocalPlayerController())
			{
				OnRep_bIsActionPhase();
			}
			else
			{
				SetActionMode(ECSKActionPhaseMode::None, true);
			}

			// If its our action phase, we should always be able to move.
			// We can check if we can build or destroy any towers before setting build actions
			// TODO: Check if we have enough mana to cast at least one spell (and those spells can be used during action phase)
			ECSKActionPhaseMode ModesToEnable = bEnabled ? ECSKActionPhaseMode::MoveCastle | ECSKActionPhaseMode::CastSpell : ECSKActionPhaseMode::None;

			if (bEnabled)
			{
				// Reset tiles traversed
				ACSKPlayerState* PlayerState = GetCSKPlayerState();
				if (PlayerState)
				{
					PlayerState->ResetTilesTraversed();
					PlayerState->ResetSpellsCast();
				}

				// Check if we can build or destroy towers
				ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
				if (GameState && GameState->CanPlayerBuildMoreTowers(this, true))
				{
					ModesToEnable |= ECSKActionPhaseMode::BuildTowers;
				}
			}

			RemainingActions = ModesToEnable;
		}
	}
}

void ACSKPlayerController::EndActionPhase()
{
	if (IsLocalPlayerController() && CanEndActionPhase())
	{
		Server_EndActionPhase();
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

bool ACSKPlayerController::CanEndActionPhase() const
{
	if (IsPerformingActionPhase())
	{
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (GameState)
		{
			return GameState->HasPlayerMovedRequiredTiles(this);
		}

		// If movement is being tracked, just assume we can end action phase whenever
		return true;
	}

	return false;
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

bool ACSKPlayerController::CanRequestBuildTowerAction() const
{
	if (IsPerformingActionPhase() && EnumHasAnyFlags(SelectedAction, ECSKActionPhaseMode::BuildTowers))
	{
		// We need to know where we are on the map to determine if target tile is within build range
		return CastlePawn != nullptr;
	}

	return false;
}

bool ACSKPlayerController::CanRequestCastSpellAction() const
{
	if (IsPerformingActionPhase() && EnumHasAnyFlags(SelectedAction, ECSKActionPhaseMode::CastSpell))
	{
		return true;
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
			Pawn->TravelToLocation(CastlePawn->GetActorLocation(), true);
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

void ACSKPlayerController::Client_OnTowerBuildRequestConfirmed_Implementation(ATile* TargetTile)
{
	bCanSelectTile = false;

	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn && TargetTile)
	{
		// Have players watch get tower built		
		Pawn->TravelToLocation(TargetTile->GetActorLocation(), false);
		SetIgnoreMoveInput(true);
	}
}

void ACSKPlayerController::Client_OnTowerBuildRequestFinished_Implementation()
{
	bCanSelectTile = true;
	SetIgnoreMoveInput(false);
}

void ACSKPlayerController::Client_OnCastSpellRequestConfirmed_Implementation(ATile* TargetTile)
{
	bCanSelectTile = false;

	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn && TargetTile)
	{
		// Have players watch spell in action	
		Pawn->TravelToLocation(TargetTile->GetActorLocation(), false);
		SetIgnoreMoveInput(true);
	}
}

void ACSKPlayerController::Client_OnCastSpellRequestFinished_Implementation()
{
	bCanSelectTile = true;
	SetIgnoreMoveInput(false);
}

bool ACSKPlayerController::DisableActionMode(ECSKActionPhaseMode ActionMode)
{
	if (HasAuthority() && IsPerformingActionPhase())
	{
		if (ActionMode != ECSKActionPhaseMode::None || ActionMode != ECSKActionPhaseMode::All)
		{
			RemainingActions &= ~ActionMode;
			return RemainingActions == ECSKActionPhaseMode::None;
		}
	}

	return false;
}

bool ACSKPlayerController::Server_EndActionPhase_Validate()
{
	return true;
}

void ACSKPlayerController::Server_EndActionPhase_Implementation()
{
	bool bSuccess = false;

	if (CanEndActionPhase())
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestEndActionPhase();
		}
	}

	// TODO: notify client if we failed (we prob want to send a reason as well)
	if (!bSuccess)
	{

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

bool ACSKPlayerController::Server_RequestBuildTowerAction_Validate(TSubclassOf<UTowerConstructionData> TowerConstructData, ATile* Target)
{
	return true;
}

void ACSKPlayerController::Server_RequestBuildTowerAction_Implementation(TSubclassOf<UTowerConstructionData> TowerConstructData, ATile* Target)
{
	bool bSuccess = false;

	if (CanRequestBuildTowerAction())
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestBuildTower(TowerConstructData, Target);
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}

bool ACSKPlayerController::Server_RequestCastSpellAction_Validate(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana)
{
	return true;
}

void ACSKPlayerController::Server_RequestCastSpellAction_Implementation(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana)
{
	bool bSuccess = false;

	if (CanRequestCastSpellAction())
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestCastSpell(SpellCard, SpellIndex, Target, AdditionalMana);
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}