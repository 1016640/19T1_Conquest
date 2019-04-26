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
#include "Spell.h"
#include "SpellCard.h"
#include "Tower.h"
#include "TowerConstructionData.h"

#include "TimerManager.h"
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
	bCanSelectNullifyQuickEffect = false;
	bCanSelectPostQuickEffect = false;
	bCanSelectBonusSpellTarget = false;
	bIgnoreCanSelectSpellFlags = false;
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
			GameState->SetLocalPlayersPawn(GetCSKPawn());
			GameState->OnRoundStateChanged.AddDynamic(this, &ACSKPlayerController::OnRoundStateChanged);
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKPlayerController: Unable to bind round state "
				"change event and set local player pawn as game state is not of CSKGameState"));
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
		// We only want to notify tiles once the game has actually begun (as is running)
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (GameState && GameState->IsMatchInProgress())
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

				OnNewTileHovered(HoveredTile);

				// Notify HUD, HUD will handle null checks
				if (CachedCSKHUD)
				{
					CachedCSKHUD->OnTileHovered(HoveredTile);
				}
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
	
	// We don't need to specify Owner only as each client will only have access to their controller
	DOREPLIFETIME(ACSKPlayerController, CSKPlayerID);
	DOREPLIFETIME(ACSKPlayerController, CastlePawn);
	DOREPLIFETIME(ACSKPlayerController, bIsActionPhase);
	DOREPLIFETIME(ACSKPlayerController, RemainingActions);
	DOREPLIFETIME(ACSKPlayerController, bCanSelectNullifyQuickEffect);
	DOREPLIFETIME(ACSKPlayerController, bCanSelectPostQuickEffect);
	DOREPLIFETIME(ACSKPlayerController, bCanSelectBonusSpellTarget);
}

void ACSKPlayerController::SetSelectedTower(TSubclassOf<UTowerConstructionData> InConstructData)
{
	if (IsLocalPlayerController() && IsPerformingActionPhase())
	{
		SelectedTowerConstructionData = InConstructData;
	}
}

void ACSKPlayerController::SetSelectedSpellCard(TSubclassOf<USpellCard> InSpellCard, int32 InSpellIndex)
{
	if (IsLocalPlayerController() /*&&*/)
	{
		SelectedSpellCard = InSpellCard;
		
		const USpellCard* DefaultSpellCard = SelectedSpellCard ? SelectedSpellCard.GetDefaultObject() : nullptr;
		if (DefaultSpellCard && DefaultSpellCard->GetSpellAtIndex(InSpellIndex))
		{
			SelectedSpellIndex = InSpellIndex;

			// Stop here if ignoring spell selections
			if (bIgnoreCanSelectSpellFlags)
			{
				return;
			}

			// If the selected spell doesn't require a target, we can simply cast the spell now instead of waiting or a tile to be selected
			const USpell* DefaultSpell = DefaultSpellCard->GetSpellAtIndex(SelectedSpellIndex).GetDefaultObject();
			if (!DefaultSpell->RequiresTarget())
			{
				ATile* TargetTile = CastlePawn ? CastlePawn->GetCachedTile() : nullptr;
				if (IsPerformingActionPhase())
				{
					Server_RequestCastSpellAction(SelectedSpellCard, SelectedSpellIndex, TargetTile, SelectedSpellAdditionalMana);
					
				}
				else if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect)
				{
					Server_RequestCastQuickEffectAction(SelectedSpellCard, SelectedSpellIndex, TargetTile, SelectedSpellAdditionalMana);
				}
			}
		}
		else
		{
			SelectedSpellIndex = 0;
		}
	}
}

void ACSKPlayerController::SetSelectedAdditionalMana(int32 InAdditionalMana)
{
	if (IsLocalPlayerController() && IsPerformingActionPhase())
	{
		SelectedSpellAdditionalMana = FMath::Max(0, InAdditionalMana);
	}
}

bool ACSKPlayerController::HasTowerSelected(TSubclassOf<UTowerConstructionData> InConstructData) const
{
	if (SelectedTowerConstructionData && InConstructData)
	{
		return SelectedTowerConstructionData == InConstructData;
	}

	return false;
}

bool ACSKPlayerController::HasSpellSelected(TSubclassOf<USpell> InSpell) const
{
	if (SelectedSpellCard && InSpell)
	{
		const USpellCard* DefaultSpellCard = SelectedSpellCard.GetDefaultObject();
		check(DefaultSpellCard);

		TSubclassOf<USpell> SelectedSpell = DefaultSpellCard->GetSpellAtIndex(SelectedSpellIndex);
		if (SelectedSpell)
		{
			return SelectedSpell == InSpell;
		}
	}

	return false;
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

void ACSKPlayerController::OnNewTileHovered_Implementation(ATile* NewTile)
{
	check(IsLocalPlayerController());

	// Either a spell actor or a tower has bound to input, we
	// can show if the action can be executed similar to spells
	// We check this first as we prioritize spell actor bindings
	// over spell cast ones (since spell actor is more relevant)
	if (CustomCanSelectTile.IsBound())
	{
		SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);

		// Tile will be null if no longer hovering over the board
		if (NewTile)
		{
			// We only display the hovered tile to be selectable
			SelectedActionTileCandidates.Empty(1);
			SelectedActionTileCandidates.Add(NewTile);

			if (CustomCanSelectTile.Execute(NewTile))
			{
				NewTile->SetSelectionState(ETileSelectionState::SelectablePriority);
			}
			else
			{
				NewTile->SetSelectionState(ETileSelectionState::UnselectablePriority);
			}
		}
		else
		{
			SelectedActionTileCandidates.Empty();
		}

		return;
	}

	bool bDisplaySpellSelection = false;

	// We want to update the selectable tile based off our current spell we have selected
	if (IsPerformingActionPhase())
	{
		bDisplaySpellSelection = SelectedAction == ECSKActionPhaseMode::CastSpell;
	}
	else if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect || bCanSelectBonusSpellTarget)
	{
		bDisplaySpellSelection = true;
	}

	if (bDisplaySpellSelection)
	{
		SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);

		// Tile will be null if no longer hovering over the board
		if (NewTile)
		{
			// We only display the hovered tile to be selectable
			SelectedActionTileCandidates.Empty(1);
			SelectedActionTileCandidates.Add(NewTile);

			ACSKGameState* CSKGameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (CSKGameState)
			{
				// We want the selectable highlight to take priority over 
				ETileSelectionState SelectionState = CSKGameState->CanPlayerCastSpell(this, NewTile, SelectedSpellCard, SelectedSpellIndex, SelectedSpellAdditionalMana)
					? ETileSelectionState::SelectablePriority : ETileSelectionState::UnselectablePriority;

				NewTile->SetSelectionState(SelectionState);
			}
		}
		else
		{
			SelectedActionTileCandidates.Empty();
		}
	}
}

void ACSKPlayerController::SetCanSelectTile(bool bEnable)
{
	if (bCanSelectTile != bEnable)
	{
		bCanSelectTile = bEnable;

		if (!bCanSelectTile)
		{

		}
	}
}

void ACSKPlayerController::SelectTile()
{
	if (!bCanSelectTile || !HoveredTile)
	{
		return;
	}

	// We have to check this before hand, as players can use bonus
	// spells both during their action phase and while waiting
	if (!bIgnoreCanSelectSpellFlags && bCanSelectBonusSpellTarget)
	{
		CastBonusElementalSpellAtHoveredTile();
		return;
	}

	// Are we allowed to perform actions
	if (IsPerformingActionPhase())
	{
		switch (SelectedAction)
		{
			case ECSKActionPhaseMode::MoveCastle:
			{
				MoveCastleToHoveredTile();
				return;
			}
			case ECSKActionPhaseMode::BuildTowers:
			{
				BuildTowerAtHoveredTile(SelectedTowerConstructionData);
				return;
			}
			case ECSKActionPhaseMode::CastSpell:
			{
				if (!bIgnoreCanSelectSpellFlags)
				{
					CastSpellAtHoveredTile(SelectedSpellCard, SelectedSpellIndex, SelectedSpellAdditionalMana);
					return;
				}
				break;
			}
		}
	}
	
	// We can check this after as players can only perform
	// quick effect attacks when not performing their action phase
	if (!bIgnoreCanSelectSpellFlags && (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect))
	{
		CastQuickEffectSpellAtHoveredTile(SelectedSpellCard, SelectedSpellIndex, SelectedSpellAdditionalMana);
		return;
	}

	// Execute custom selection last. We check CanSelect both on client and the server
	// (We can skip the check here completely if we are the server to prevent the check twice)
	if (HasAuthority() || (!CustomCanSelectTile.IsBound() || CustomCanSelectTile.Execute(HoveredTile)))
	{
		Server_ExecuteCustomOnSelectTile(HoveredTile);
		return;
	}
}

void ACSKPlayerController::ResetCamera()
{
	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn && CastlePawn)
	{
		CSKPawn->TravelToLocation(CastlePawn->GetActorLocation());
	}
}

bool ACSKPlayerController::Server_ExecuteCustomOnSelectTile_Validate(ATile* SelectedTile)
{
	return true;
}

void ACSKPlayerController::Server_ExecuteCustomOnSelectTile_Implementation(ATile* SelectedTile)
{
	// We can skip the can select if it's not bound (assume it returns true)
	if (!CustomCanSelectTile.IsBound() || CustomCanSelectTile.Execute(SelectedTile))
	{
		if (CustomOnSelectTile.IsBound())
		{
			CustomOnSelectTile.Execute(SelectedTile);
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

		ACSKPlayerState* CSKPlayerState = GetCSKPlayerState();
		if (CSKPlayerState)
		{
			CSKPlayerState->SetCastle(CastlePawn);
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

			ACSKPawn* CSKPawn = GetCSKPawn();
			if (CSKPawn && CastlePawn)
			{
				// Focus on our castle while we wait for tallied resources
				CSKPawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
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
			// We may have been waiting previously, and 
			// we don't want to stack ignore commands
			if (!IsMoveInputIgnored())
			{
				SetIgnoreMoveInput(true);
			}

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

void ACSKPlayerController::Client_OnMatchFinished_Implementation(bool bIsWinner)
{
	SetCanSelectTile(false);

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnMatchFinished(bIsWinner);
	}
}

void ACSKPlayerController::Client_OnCollectionPhaseResourcesTallied_Implementation(FCollectionPhaseResourcesTally TalliedResources)
{
	bWaitingOnTallyEvent = true;
	OnCollectionResourcesTallied(TalliedResources);
}

void ACSKPlayerController::OnCollectionResourcesTallied_Implementation(const FCollectionPhaseResourcesTally& TalliedResources)
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimerForNextTick(this, &ACSKPlayerController::FinishCollectionSequenceEvent);
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
			// We can check if we can build or destroy any towers before setting build actions. We can do the same with spells
			ECSKActionPhaseMode ModesToEnable = bEnabled ? ECSKActionPhaseMode::MoveCastle : ECSKActionPhaseMode::None;

			if (bEnabled)
			{
				// Reset tiles traversed
				ACSKPlayerState* CSKPlayerState = GetCSKPlayerState();
				if (CSKPlayerState)
				{
					CSKPlayerState->ResetTilesTraversed();
					CSKPlayerState->ResetSpellsCast();

					// Check not only if we can cast a spell, but if we can afford any of them
					if (CSKPlayerState->CanCastAnotherSpell(true))
					{
						ModesToEnable |= ECSKActionPhaseMode::CastSpell;
					}
				}

				// Check if we can build or destroy towers
				ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
				if (GameState && GameState->CanPlayerBuildMoreTowers(this))
				{
					ModesToEnable |= ECSKActionPhaseMode::BuildTowers;
				}
			}
			else
			{
				bCanSelectNullifyQuickEffect = false;
				bCanSelectPostQuickEffect = false;
				bCanSelectBonusSpellTarget = false;
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

			if (CachedCSKHUD)
			{
				CachedCSKHUD->OnSelectedActionChanged(NewMode);
			}
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

void ACSKPlayerController::SetNullifyQuickEffectSelectionEnabled(bool bEnable)
{
	if (HasAuthority() && bCanSelectNullifyQuickEffect != bEnable)
	{
		bCanSelectNullifyQuickEffect = bEnable;

		if (IsLocalPlayerController())
		{
			OnRep_bCanSelectNullifyQuickEffect();
		}
	}
}

void ACSKPlayerController::SetPostQuickEffectSelectionEnabled(bool bEnable)
{
	if (HasAuthority() && bCanSelectPostQuickEffect != bEnable)
	{
		bCanSelectPostQuickEffect = bEnable;

		if (IsLocalPlayerController())
		{
			OnRep_bCanSelectPostQuickEffect();
		}
	}
}

void ACSKPlayerController::ResetQuickEffectSelections()
{
	SetNullifyQuickEffectSelectionEnabled(false);
	SetPostQuickEffectSelectionEnabled(false);
}

void ACSKPlayerController::SetBonusSpellSelectionEnabled(bool bEnable)
{
	if (HasAuthority() && bCanSelectBonusSpellTarget != bEnable)
	{
		bCanSelectBonusSpellTarget = bEnable;

		if (IsLocalPlayerController())
		{
			OnRep_bCanSelectBonusSpellTarget();
		}
	}
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

		// If movement isn't being tracked, just assume we can end action phase whenever
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

void ACSKPlayerController::OnSelectionModeChanged_Implementation(ECSKActionPhaseMode NewMode)
{
	// Mark previous tiles as disabled
	SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);
	SelectedActionTileCandidates.Empty();

	ACSKGameState* CSKGameState = UConquestFunctionLibrary::GetCSKGameState(this);
	if (CSKGameState)
	{
		switch (NewMode)
		{
			case ECSKActionPhaseMode::MoveCastle:
			{
				ensure(CSKGameState->GetTilesPlayerCanMoveTo(this, SelectedActionTileCandidates, true));
				break;
			}
			case ECSKActionPhaseMode::BuildTowers:
			{
				CSKGameState->GetTilesPlayerCanBuildOn(this, SelectedActionTileCandidates);
				break;
			}
		}
	}

	// All candidates are selectable, but we want to display the hover highlight over it
	SetTileCandidatesSelectionState(ETileSelectionState::Selectable);
}

void ACSKPlayerController::OnRep_bIsActionPhase()
{
	SetActionMode(ECSKActionPhaseMode::None, true);

	// Move our camera to our castle when it's our turn to make actions
	if (bIsActionPhase)
	{
		ACSKPawn* CSKPawn = GetCSKPawn();
		if (CSKPawn && CastlePawn)
		{
			CSKPawn->TravelToLocation(CastlePawn->GetActorLocation(), true);
		}

		SetCanSelectTile(true);
	}
	else
	{
		SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);
		SelectedActionTileCandidates.Empty();
	}

	// Always reset the selected data
	{
		SelectedTowerConstructionData = nullptr;
		SelectedSpellCard = nullptr;
		SelectedSpellIndex = 0;
		SelectedSpellAdditionalMana = 0;
	}
}

void ACSKPlayerController::OnRep_RemainingActions()
{
	if (!EnumHasAllFlags(RemainingActions, SelectedAction))
	{
		SetActionMode(ECSKActionPhaseMode::None, true);
	}
}

void ACSKPlayerController::OnRep_bCanSelectNullifyQuickEffect()
{
	if (bCanSelectNullifyQuickEffect)
	{
		SetCanSelectTile(true);

		// In this case, this should already be null
		bIgnoreCanSelectSpellFlags = false;
	}
	else if (ensure(!IsPerformingActionPhase()))
	{
		SetCanSelectTile(false);
	}
}

void ACSKPlayerController::OnRep_bCanSelectPostQuickEffect()
{
	if (bCanSelectPostQuickEffect)
	{
		SetCanSelectTile(true);

		// We can safely disable this again as no spell is active right now
		bIgnoreCanSelectSpellFlags = false;
	}
	else if (ensure(!IsPerformingActionPhase()))
	{
		SetCanSelectTile(false);
	}
}

void ACSKPlayerController::OnRep_bCanSelectBonusSpellTarget()
{
	if (bCanSelectBonusSpellTarget)
	{
		SetCanSelectTile(true);

		// We can safely disable this again as no spell is active right now
		bIgnoreCanSelectSpellFlags = false;
	}
	else if (!IsPerformingActionPhase())
	{
		SetCanSelectTile(false);
	}
}

void ACSKPlayerController::Client_OnTransitionToBoard_Implementation()
{
	// Move the camera over to our castle (where our portal should be)
	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn)
	{
		if (CastlePawn)
		{
			CSKPawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
		}
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("Client was unable to travel to castle as pawn was null! (Probaly not replicated yet)"));
	}
}

void ACSKPlayerController::Client_OnCastleMoveRequestConfirmed_Implementation(ACastle* MovingCastle)
{
	SetCanSelectTile(false);

	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn)
	{
		CSKPawn->TrackActor(MovingCastle);
	}

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionStart(ECSKActionPhaseMode::MoveCastle, EActiveSpellContext::None);
	}

	// Mark tiles as not selectable while we move
	if (IsPerformingActionPhase())
	{
		SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);
	}
}

void ACSKPlayerController::Client_OnCastleMoveRequestFinished_Implementation()
{
	SetCanSelectTile(true);

	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn)
	{
		CSKPawn->TrackActor(nullptr);
	}

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionFinished(ECSKActionPhaseMode::MoveCastle, EActiveSpellContext::None);
	}

	// If it's our turn, we may still be able to move some more tiles,
	// so we need to refresh the amount of tiles we can move
	// TODO: Need to wait for TilesTraversedThisRound to replicate on the client (for doing this on the client)
	if (IsPerformingActionPhase() && SelectedAction == ECSKActionPhaseMode::MoveCastle)
	{
		ACSKGameState* CSKGameState = UConquestFunctionLibrary::GetCSKGameState(this);
		check(CSKGameState);

		if (CSKGameState->GetTilesPlayerCanMoveTo(this, SelectedActionTileCandidates, true))
		{
			SetTileCandidatesSelectionState(ETileSelectionState::Selectable);
		}
	}
}

void ACSKPlayerController::Client_OnTowerBuildRequestConfirmed_Implementation(ATile* TargetTile)
{
	SetCanSelectTile(false);

	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn && TargetTile)
	{
		// Have players watch get tower built		
		CSKPawn->TravelToLocation(TargetTile->GetActorLocation(), false);
		SetIgnoreMoveInput(true);
	}

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionStart(ECSKActionPhaseMode::BuildTowers, EActiveSpellContext::None);
	}

	// Mark tiles as not selectable while we build
	if (IsPerformingActionPhase())
	{
		SetTileCandidatesSelectionState(ETileSelectionState::NotSelectable);
	}
}

void ACSKPlayerController::Client_OnTowerBuildRequestFinished_Implementation()
{
	SetCanSelectTile(true);
	SetIgnoreMoveInput(false);

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionFinished(ECSKActionPhaseMode::BuildTowers, EActiveSpellContext::None);
	}

	// If it's our turn, we may still be able to build more towers,
	// so we need to refresh which tiles are considered selectable
	if (IsPerformingActionPhase() && SelectedAction == ECSKActionPhaseMode::BuildTowers)
	{
		ACSKGameState* CSKGameState = UConquestFunctionLibrary::GetCSKGameState(this);
		check(CSKGameState);

		if (CSKGameState->GetTilesPlayerCanBuildOn(this, SelectedActionTileCandidates))
		{
			SetTileCandidatesSelectionState(ETileSelectionState::Selectable);
		}
	}
}

void ACSKPlayerController::Client_OnCastSpellRequestConfirmed_Implementation(EActiveSpellContext SpellContext, ATile* TargetTile)
{
	SetCanSelectTile(false);

	// We want to ignore any spell selections at this point
	bIgnoreCanSelectSpellFlags = true;

	// Avoid setting it twice, as this function will get
	// called twice before Finish if casting a bonus spell
	if (!IsMoveInputIgnored())
	{
		SetIgnoreMoveInput(true);
	}

	ACSKPawn* CSKPawn = GetCSKPawn();
	if (CSKPawn && TargetTile)
	{
		// Have players watch spell in action	
		CSKPawn->TravelToLocation(TargetTile->GetActorLocation(), false);
	}

	// This can be reset here safely
	PendingBonusSpell = nullptr;

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionStart(ECSKActionPhaseMode::CastSpell, SpellContext);
	}

	SelectedSpellCard = nullptr;
	SelectedSpellIndex = 0;
}

void ACSKPlayerController::Client_OnCastSpellRequestFinished_Implementation(EActiveSpellContext SpellContext)
{
	SetCanSelectTile(true);
	SetIgnoreMoveInput(false);

	// We may be able to select another spell again
	bIgnoreCanSelectSpellFlags = false;

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnActionFinished(ECSKActionPhaseMode::CastSpell, SpellContext);
	}
}

void ACSKPlayerController::Client_OnSelectCounterSpell_Implementation(bool bNullify, TSubclassOf<USpell> SpellToCounter, ATile* TargetTile)
{
	SetCanSelectTile(true);
	SetIgnoreMoveInput(false);

	if (CachedCSKHUD)
	{
		const USpell* DefaultSpell = SpellToCounter.GetDefaultObject();
		CachedCSKHUD->OnQuickEffectSelection(true, bNullify, DefaultSpell, TargetTile);
	}
}

void ACSKPlayerController::Client_OnWaitForCounterSpell_Implementation(bool bNullify)
{
	SetCanSelectTile(false);
	SetIgnoreMoveInput(false);

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnQuickEffectSelection(false, bNullify, nullptr, nullptr);
	}
}

void ACSKPlayerController::Client_OnSelectBonusSpellTarget_Implementation(TSubclassOf<USpell> BonusSpell)
{
	SetCanSelectTile(true);
	SetIgnoreMoveInput(false);

	PendingBonusSpell = BonusSpell;

	if (CachedCSKHUD)
	{
		const USpell* DefaultSpell = BonusSpell.GetDefaultObject();
		CachedCSKHUD->OnBonusSpellSelection(true, DefaultSpell);
	}
}

void ACSKPlayerController::Client_OnWaitForBonusSpell_Implementation()
{
	SetCanSelectTile(false);
	SetIgnoreMoveInput(false);

	if (CachedCSKHUD)
	{
		CachedCSKHUD->OnBonusSpellSelection(false, nullptr);
	}
}

bool ACSKPlayerController::DisableActionMode(ECSKActionPhaseMode ActionMode)
{
	if (HasAuthority() && IsPerformingActionPhase())
	{
		if (ActionMode != ECSKActionPhaseMode::None || ActionMode != ECSKActionPhaseMode::All)
		{
			RemainingActions &= ~ActionMode;

			// This is needed for listen server matches
			if (IsLocalPlayerController())
			{
				OnRep_RemainingActions();
			}

			return RemainingActions == ECSKActionPhaseMode::None;
		}
	}

	return false;
}

void ACSKPlayerController::MoveCastleToHoveredTile()
{
	// Only local players should build
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (IsPerformingActionPhase() && SelectedAction == ECSKActionPhaseMode::MoveCastle)
	{
		if (HoveredTile)
		{
			Server_RequestCastleMoveAction(HoveredTile);
		}
	}
}

void ACSKPlayerController::BuildTowerAtHoveredTile(TSubclassOf<UTowerConstructionData> TowerConstructData)
{
	// Only local players should build
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (IsPerformingActionPhase() && SelectedAction == ECSKActionPhaseMode::BuildTowers)
	{
		if (HoveredTile)
		{
			Server_RequestBuildTowerAction(TowerConstructData, HoveredTile);
		}
	}
}

void ACSKPlayerController::CastSpellAtHoveredTile(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, int32 AdditionalMana)
{
	// Only local players should cast
	if (!IsLocalPlayerController() && bIgnoreCanSelectSpellFlags)
	{
		return;
	}

	if (IsPerformingActionPhase() && SelectedAction == ECSKActionPhaseMode::CastSpell)
	{
		if (HoveredTile)
		{
			Server_RequestCastSpellAction(SpellCard, SpellIndex, HoveredTile, AdditionalMana);
		}
	}
}

void ACSKPlayerController::CastQuickEffectSpellAtHoveredTile(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, int32 AdditionalMana)
{
	// Only local players should cast
	if (!IsLocalPlayerController() && bIgnoreCanSelectSpellFlags)
	{
		return;
	}

	if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect)
	{
		if (HoveredTile)
		{
			Server_RequestCastQuickEffectAction(SpellCard, SpellIndex, HoveredTile, AdditionalMana);
		}
	}
}

void ACSKPlayerController::SkipQuickEffectSpell()
{
	// Only local players should cast
	if (!IsLocalPlayerController() && bIgnoreCanSelectSpellFlags)
	{
		return;
	}

	if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect)
	{
		Server_SkipQuickEffectSelection();
	}
}

void ACSKPlayerController::CastBonusElementalSpellAtHoveredTile()
{
	// Only local players should cast
	if (!IsLocalPlayerController() && bIgnoreCanSelectSpellFlags)
	{
		return;
	}

	if (bCanSelectBonusSpellTarget)
	{
		if (HoveredTile)
		{
			Server_RequestCastBonusSpellAction(HoveredTile);
		}
	}
}

void ACSKPlayerController::SkipBonusElementalSpell()
{
	// Only local players should cast
	if (!IsLocalPlayerController() && bIgnoreCanSelectSpellFlags)
	{
		return;
	}

	if (bCanSelectBonusSpellTarget)
	{
		Server_SkipBonusSpellSelection();
	}
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

bool ACSKPlayerController::Server_RequestCastQuickEffectAction_Validate(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana)
{
	return true;
}

void ACSKPlayerController::Server_RequestCastQuickEffectAction_Implementation(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* Target, int32 AdditionalMana)
{
	bool bSuccess = false;

	if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestCastQuickEffect(SpellCard, SpellIndex, Target, AdditionalMana);
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}

bool ACSKPlayerController::Server_SkipQuickEffectSelection_Validate()
{
	return true;
}

void ACSKPlayerController::Server_SkipQuickEffectSelection_Implementation()
{
	bool bSuccess = false;

	if (bCanSelectNullifyQuickEffect || bCanSelectPostQuickEffect)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestSkipQuickEffect();
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}

bool ACSKPlayerController::Server_RequestCastBonusSpellAction_Validate(ATile* Target)
{
	return true;
}

void ACSKPlayerController::Server_RequestCastBonusSpellAction_Implementation(ATile* Target)
{
	bool bSuccess = false;

	if (bCanSelectBonusSpellTarget)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestCastBonusSpell(Target);
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}

bool ACSKPlayerController::Server_SkipBonusSpellSelection_Validate()
{
	return true;
}

void ACSKPlayerController::Server_SkipBonusSpellSelection_Implementation()
{
	bool bSuccess = false;

	if (bCanSelectBonusSpellTarget)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bSuccess = GameMode->RequestSkipBonusSpell();
		}
	}

	// TODO: inform client if we failed
	if (!bSuccess)
	{

	}
}

void ACSKPlayerController::GetBuildableTowers(TArray<TSubclassOf<UTowerConstructionData>>& OutTowers) const
{
	ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
	if (GameState)
	{
		GameState->GetTowersPlayerCanBuild(this, OutTowers);
	}
}

void ACSKPlayerController::GetCastableSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards) const
{
	ACSKPlayerState* CSKPlayerState = GetCSKPlayerState();
	if (CSKPlayerState)
	{
		CSKPlayerState->GetSpellsPlayerCanCast(OutSpellCards);
	}
}

void ACSKPlayerController::GetCastableQuickEffectSpells(TArray<TSubclassOf<USpellCard>>& OutSpellCards, bool bNullifySpells) const
{
	ACSKPlayerState* CSKPlayerState = GetCSKPlayerState();
	if (CSKPlayerState)
	{
		CSKPlayerState->GetQuickEffectSpellsPlayerCanCast(OutSpellCards, bNullifySpells);
	}
}

void ACSKPlayerController::SetTileCandidatesSelectionState(ETileSelectionState SelectionState) const
{
	for (ATile* Tile : SelectedActionTileCandidates)
	{
		// Tiles should always be valid, this is a safety check in case
		if (ensure(Tile))
		{
			Tile->SetSelectionState(SelectionState);
		}
	}
}
