// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameState.h"
#include "CSKGameMode.h"
#include "CSKPawn.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "BoardManager.h"
#include "BoardPathFollowingComponent.h"
#include "Castle.h"
#include "CastleAIController.h"
#include "Spell.h"
#include "SpellCard.h"
#include "Tower.h"
#include "TowerConstructionData.h"
#include "Engine/World.h"

DECLARE_CYCLE_STAT(TEXT("ACSKGameState GetTilesPlayerCanMoveTo Pathfind"), STAT_CSKGameStateGetTilesPlayerCanMoveToPathfind, STATGROUP_Conquest);

ACSKGameState::ACSKGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BoardManager = nullptr;

	MatchState = ECSKMatchState::EnteringGame;
	RoundState = ECSKRoundState::Invalid;
	PreviousMatchState = MatchState;
	PreviousRoundState = RoundState;
	MatchWinnerPlayerID = -1;
	MatchWinCondition = ECSKMatchWinCondition::Unknown;

	CoinTossWinnerPlayerID = -1;
	TimerState = ECSKTimerState::None;
	TimeRemaining = 0;
	bTimerPaused = false;

	ActionPhaseTime = 90.f;
	MaxNumTowers = 7;
	MaxNumDuplicatedTowers = 2;
	MaxNumLegendaryTowers = 1;
	MinTileMovements = 1;
	MaxTileMovements = 2;

	RoundsPlayed = 0;
}

void ACSKGameState::OnRep_ReplicatedHasBegunPlay()
{
	if (bReplicatedHasBegunPlay)
	{
		// We need to have a board manager, we will find one until we wait for the initial one to replicate
		if (!BoardManager)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameState: Board Manager has not been replicated to client. Finding the first available board managaer instead."));
			BoardManager = UConquestFunctionLibrary::FindMatchBoardManager(this);
		}
	}

	Super::OnRep_ReplicatedHasBegunPlay();
}

void ACSKGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACSKGameState, BoardManager, COND_InitialOnly);
	DOREPLIFETIME(ACSKGameState, MatchState);
	DOREPLIFETIME(ACSKGameState, RoundState);

	DOREPLIFETIME(ACSKGameState, CoinTossWinnerPlayerID);
	DOREPLIFETIME(ACSKGameState, TimerState);
	DOREPLIFETIME(ACSKGameState, TimeRemaining);
	DOREPLIFETIME(ACSKGameState, LatestActionHealthReports);

	DOREPLIFETIME(ACSKGameState, ActionPhaseTime);
	DOREPLIFETIME(ACSKGameState, MaxNumTowers);
	DOREPLIFETIME(ACSKGameState, MaxNumDuplicatedTowers);
	DOREPLIFETIME(ACSKGameState, MaxNumDuplicatedTowerTypes);
	DOREPLIFETIME(ACSKGameState, MaxNumLegendaryTowers);
	DOREPLIFETIME(ACSKGameState, MaxBuildRange);
	DOREPLIFETIME(ACSKGameState, AvailableTowers);
	DOREPLIFETIME(ACSKGameState, MinTileMovements);
	DOREPLIFETIME(ACSKGameState, MaxTileMovements);
}

void ACSKGameState::SetMatchBoardManager(ABoardManager* InBoardManager)
{
	if (HasAuthority() && !BoardManager)
	{
		BoardManager = InBoardManager;
	}
}

void ACSKGameState::SetLocalPlayersPawn(ACSKPawn* InPlayerPawn)
{
	if (!LocalPlayerPawn)
	{
		if (InPlayerPawn && InPlayerPawn->IsLocallyControlled())
		{
			LocalPlayerPawn = InPlayerPawn;
		}
	}
}

ABoardManager* ACSKGameState::GetBoardManager(bool bErrorCheck) const
{
	if (bErrorCheck && !ensure(BoardManager))
	{
		UE_LOG(LogConquest, Error, TEXT("CSKGameState::GetBoardManager: BoardManager is null"));
	}

	return BoardManager;
}

void ACSKGameState::SetMatchState(ECSKMatchState NewState)
{
	if (HasAuthority())
	{
		MatchState = NewState;
		HandleMatchStateChange(NewState);
	}
}

void ACSKGameState::SetRoundState(ECSKRoundState NewState)
{
	if (HasAuthority())
	{
		RoundState = NewState;
		HandleRoundStateChange(NewState);
	}
}

bool ACSKGameState::IsMatchInProgress() const
{
	return MatchState == ECSKMatchState::Running;
}

bool ACSKGameState::IsActionPhaseActive() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::FirstActionPhase || RoundState == ECSKRoundState::SecondActionPhase;
	}

	return false;
}

bool ACSKGameState::IsEndRoundPhaseActive() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::EndRoundPhase;
	}

	return false;
}

void ACSKGameState::NotifyWaitingForPlayers()
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();

	// Capture rules set out by the game mode, as
	// certain checks need to know about these values
	UpdateRules();
}

void ACSKGameState::NotifyPerformCoinFlip()
{
	MatchStartTime = GetWorld()->GetTimeSeconds();
}

void ACSKGameState::NotifyMatchStart()
{
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyMatchStarted();

	if (HasAuthority())
	{
		bReplicatedHasBegunPlay = true;

		// Game mode only exists on the server
		ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			CoinTossWinnerPlayerID = GameMode->GetStartingPlayersID();
		}
	}

	RoundsPlayed = 0;
}

void ACSKGameState::NotifyMatchFinished()
{
	MatchEndTime = GetWorld()->GetTimeSeconds();

	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		ACSKPlayerController* WinningPlayer = nullptr;
		ECSKMatchWinCondition WinCondition = ECSKMatchWinCondition::Unknown;

		// THere is a chance that there is no winner (e.g. match was abandoned)
		ensure(GameMode->GetWinnerDetails(WinningPlayer, WinCondition));
		int32 WinnerID = WinningPlayer ? WinningPlayer->CSKPlayerID : -1;

		Multi_SetWinDetails(WinnerID, WinCondition);
	}
}

void ACSKGameState::NotifyPlayersLeaving()
{
}

void ACSKGameState::NotifyMatchAbort()
{
}

void ACSKGameState::NotifyCollectionPhaseStart()
{
	++RoundsPlayed;
}

void ACSKGameState::NotifyFirstActionPhaseStart()
{
	UpdateActionPhaseProperties();
}

void ACSKGameState::NotifySecondActionPhaseStart()
{
	UpdateActionPhaseProperties();
}

void ACSKGameState::NotifyEndRoundPhaseStart()
{
	UpdateActionPhaseProperties();
}

void ACSKGameState::OnRep_MatchState()
{
	HandleMatchStateChange(MatchState);
}

void ACSKGameState::HandleMatchStateChange(ECSKMatchState NewState)
{
	// Actions performed during the pre match phase should always be run before entering other states.
	// Doing this helps people who joined late to correctly catch up to the action
	if (NewState == ECSKMatchState::WaitingPreMatch || PreviousMatchState == ECSKMatchState::EnteringGame)
	{
		NotifyWaitingForPlayers();
	}

	switch (NewState)
	{
		case ECSKMatchState::CoinFlip:
		{
			NotifyPerformCoinFlip();
			break;
		}
		case ECSKMatchState::Running:
		{
			NotifyMatchStart();
			break;
		}
		case ECSKMatchState::WaitingPostMatch:
		{
			NotifyMatchFinished();
			break;
		}
		case ECSKMatchState::LeavingGame:
		{
			NotifyPlayersLeaving();
			break;
		}
		default:
		{
			break;
		}
	}

	// Setting it same as new state, as previous state will be valid
	// after being replicated from the server (or before this call)
	PreviousMatchState = NewState;
}

void ACSKGameState::OnRep_RoundState()
{
	HandleRoundStateChange(RoundState);
}

void ACSKGameState::HandleRoundStateChange(ECSKRoundState NewState)
{
	switch (NewState)
	{
		case ECSKRoundState::CollectionPhase:
		{
			NotifyCollectionPhaseStart();
			break;
		}
		case ECSKRoundState::FirstActionPhase:
		{
			NotifyFirstActionPhaseStart();
			break;
		}
		case ECSKRoundState::SecondActionPhase:
		{
			NotifySecondActionPhaseStart();
			break;
		}
		case ECSKRoundState::EndRoundPhase:
		{
			NotifyEndRoundPhaseStart();
			break;
		}
		default:
		{
			break;
		}
	}

	// Setting it same as new state, as previous state will be valid
	// after being replicated from the server (or before this call)
	PreviousRoundState = NewState;

	OnRoundStateChanged.Broadcast(NewState);
}

void ACSKGameState::Multi_SetWinDetails_Implementation(int32 WinnerID, ECSKMatchWinCondition WinCondition)
{
	MatchWinnerPlayerID = WinnerID;
	MatchWinCondition = WinCondition;
}

ACSKPlayerState* ACSKGameState::GetPlayerStateWithID(int32 PlayerID) const
{
	for (APlayerState* Player : PlayerArray)
	{
		ACSKPlayerState* PlayerState = Cast<ACSKPlayerState>(Player);
		if (PlayerState && PlayerState->GetCSKPlayerID() == PlayerID)
		{
			return PlayerState;
		}
	}

	return nullptr;
}

ACSKPlayerState* ACSKGameState::GetOpposingPlayerState(ACSKPlayerState* Player) const
{
	if (Player)
	{
		int32 OpposingPlayerID = FMath::Abs(Player->GetCSKPlayerID() - 1);
		return GetPlayerStateWithID(OpposingPlayerID);
	}

	return nullptr;
}

bool ACSKGameState::ActivateCustomTimer(int32 InDuration)
{
	if (HasAuthority() && InDuration > 0)
	{
		// We don't allow custom timers to override the core game timers
		if (TimerState == ECSKTimerState::None || TimerState == ECSKTimerState::Custom)
		{
			ActivateTickTimer(ECSKTimerState::Custom, InDuration);
			return true;
		}
	}

	return false;
}

void ACSKGameState::DeactivateCustomTimer()
{
	if (HasAuthority() && TimerState == ECSKTimerState::Custom)
	{
		DeactivateTickTimer();
	}
}

int32 ACSKGameState::GetCountdownTimeRemaining(bool& bOutIsInfinite) const
{
	bOutIsInfinite = false;

	if (TimerState != ECSKTimerState::None)
	{
		bOutIsInfinite = TimeRemaining == -1;
		return TimeRemaining;
	}

	return 0;
}

int32 ACSKGameState::GetTowerInstanceCount(TSubclassOf<ATower> Tower) const
{
	const int32* Num = TowerInstanceTable.Find(Tower);
	if (Num != nullptr)
	{
		return *Num;
	}

	return 0;
}

void ACSKGameState::SetLatestActionHealthReports(const TArray<FHealthChangeReport>& InHealthReports)
{
	if (HasAuthority())
	{
		LatestActionHealthReports = InHealthReports;
	}
}

TArray<FHealthChangeReport> ACSKGameState::GetDamageHealthReports(bool bFilterOutDead) const
{
	return QueryLatestHealthReports(true, nullptr, bFilterOutDead);
}

TArray<FHealthChangeReport> ACSKGameState::GetHealingHealthReports() const
{
	return QueryLatestHealthReports(false, nullptr, true);
}

TArray<FHealthChangeReport> ACSKGameState::GetPlayersDamagedHealthReports(ACSKPlayerState* PlayerState, bool bFilterOutDead) const
{
	return QueryLatestHealthReports(true, PlayerState, bFilterOutDead);
}

TArray<FHealthChangeReport> ACSKGameState::GetPlayersHealingHealthReports(ACSKPlayerState* PlayerState) const
{
	return QueryLatestHealthReports(false, PlayerState, true);
}

void ACSKGameState::ActivateTickTimer(ECSKTimerState InTimerState, int32 InTime)
{
	if (HasAuthority())
	{
		if (InTimerState == ECSKTimerState::None)
		{
			DeactivateTickTimer();
			return;
		}

		// Action phase can be switched out with quick effect and bonus spell states
		if (TimerState != ECSKTimerState::None && TimerState != ECSKTimerState::ActionPhase)
		{
			UE_LOG(LogConquest, Warning, TEXT("Activating new timer state while another is already active!"))
		}

		// We save this to restore later (if required)
		if (TimerState == ECSKTimerState::ActionPhase)
		{
			NewActionPhaseTimeRemaining = TimeRemaining;
		}

		// Custom listener would appreciate being notified it was cancelled
		if (TimerState == ECSKTimerState::Custom)
		{
			ExecuteCustomTimerFinishedEvent(true);
		}

		TimerState = InTimerState;
		TimeRemaining = InTime;

		// We allow setting infinite times using -1, but
		// for this we don't need to activate the timer
		if (TimeRemaining > 0)
		{
			SetTickTimerEnabled(true);
		}
	}
}

void ACSKGameState::DeactivateTickTimer()
{
	if (HasAuthority())
	{
		TimerState = ECSKTimerState::None;
		TimeRemaining = 0;

		SetTickTimerEnabled(false);
	}
}

void ACSKGameState::SetTickTimerEnabled(bool bEnable)
{
	if (HasAuthority())
	{
		FTimerManager& TimerManager = GetWorldTimerManager();
		if (bEnable && !TimerManager.IsTimerActive(Handle_TickTimer))
		{
			AWorldSettings* WorldSettings = GetWorldSettings();
			TimerManager.SetTimer(Handle_TickTimer, this, &ACSKGameState::TickTimer, WorldSettings->GetEffectiveTimeDilation(), true);
		}
		else if (!bEnable && TimerManager.IsTimerActive(Handle_TickTimer))
		{
			TimerManager.ClearTimer(Handle_TickTimer);
		}
	}
}

int32 ACSKGameState::GetActionTimeBonusApplied(int32 Time) const
{
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		if (IsActionPhaseTimed())
		{
			return FMath::Min(ActionPhaseTime, Time + GameMode->GetBonusActionPhaseTime());
		}
		else
		{
			return -1;
		}
	}

	return ActionPhaseTime;
}

void ACSKGameState::UpdateActionPhaseProperties()
{
	// Determine which players action phase it is
	if (IsActionPhaseActive())
	{
		ActionPhasePlayerID = RoundState == ECSKRoundState::FirstActionPhase ? CoinTossWinnerPlayerID : FMath::Abs(CoinTossWinnerPlayerID - 1);
		ActivateTickTimer(ECSKTimerState::ActionPhase, ActionPhaseTime);
	}
	else
	{
		ActionPhasePlayerID = -1;
		DeactivateTickTimer();
	}
}

void ACSKGameState::TickTimer()
{
	// We should only tick on the server
	if (HasAuthority() && !bTimerPaused)
	{
		TimeRemaining -= 1;
		if (TimeRemaining <= 0)
		{
			HandleTickTimerFinished();
		}
	}
}

void ACSKGameState::HandleTickTimerFinished()
{
	ensure(TimeRemaining <= 0);

	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		// Need to save this here as deactivate will clear it
		ECSKTimerState FinishedTimer = TimerState;

		// Deactivate it now, as some of these events 
		// might result in another timer being set
		DeactivateTickTimer();

		switch (FinishedTimer)
		{
			case ECSKTimerState::ActionPhase:
			{
				GameMode->RequestEndActionPhase(true);
				break;
			}
			case ECSKTimerState::QuickEffect:
			{
				GameMode->RequestSkipQuickEffect();
				break;
			}
			case ECSKTimerState::BonusSpell:
			{
				GameMode->RequestSkipBonusSpell();
				break;
			}
			case ECSKTimerState::Custom:
			{
				ExecuteCustomTimerFinishedEvent(false);
				break;
			}
		}
	}
}

void ACSKGameState::ExecuteCustomTimerFinishedEvent(bool bWasSkipped)
{
	if (CustomTimerFinishedEvent.IsBound())
	{
		CustomTimerFinishedEvent.Execute(bWasSkipped);
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("ACSKGameState: CustomTimerFinishedEvent "
			"has not been bound even though timer state is set to custom!"));
	}
}

TArray<FHealthChangeReport> ACSKGameState::QueryLatestHealthReports(bool bDamaged, ACSKPlayerState* InOwner, bool bExcludeDead) const
{
	TArray<FHealthChangeReport> Reports;
	for (const FHealthChangeReport& Repo : LatestActionHealthReports)
	{
		// Filter owner
		if (InOwner)
		{
			if (Repo.Owner != InOwner)
			{
				continue;
			}
		}

		// Filter out differen type
		if (bDamaged != Repo.bWasDamaged)
		{
			continue;
		}

		// Only filter killed if checking damaged
		if (bDamaged)
		{
			if (bExcludeDead && Repo.bKilled)
			{
				continue;
			}
		}

		Reports.Add(Repo);
	}

	return Reports;
}

void ACSKGameState::HandleMoveRequestConfirmed()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		NewActionPhaseTimeRemaining = TimeRemaining;
		DeactivateTickTimer();

		Multi_HandleMoveRequestConfirmed();
	}
}

void ACSKGameState::HandleMoveRequestFinished()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		ActivateTickTimer(ECSKTimerState::ActionPhase, GetActionTimeBonusApplied(NewActionPhaseTimeRemaining));

		Multi_HandleMoveRequestFinished();
	}
}

void ACSKGameState::HandleBuildRequestConfirmed(ATile* TargetTile)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		NewActionPhaseTimeRemaining = TimeRemaining;
		DeactivateTickTimer();

		Multi_HandleBuildRequestConfirmed(TargetTile);
	}
}

void ACSKGameState::HandleBuildRequestFinished(ATower* NewTower)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		ActivateTickTimer(ECSKTimerState::ActionPhase, GetActionTimeBonusApplied(NewActionPhaseTimeRemaining));

		Multi_HandleBuildRequestFinished(NewTower);
	}
}

void ACSKGameState::HandleSpellRequestConfirmed(EActiveSpellContext Context, ATile* TargetTile)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// This function can get called multiple times during a spell action (up to 3 times).
		// We use timer state instead of context as it's possible to skip quick effect selections,
		// which would result in time remaining representing the quick effect selection
		/*if (TimerState == ECSKTimerState::ActionPhase)
		{
			NewActionPhaseTimeRemaining = TimeRemaining;
		}*/

		DeactivateTickTimer();

		Multi_HandleSpellRequestConfirmed(Context, TargetTile);
	}
}

void ACSKGameState::HandleSpellRequestFinished(EActiveSpellContext Context)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		ActivateTickTimer(ECSKTimerState::ActionPhase, GetActionTimeBonusApplied(NewActionPhaseTimeRemaining));

		Multi_HandleSpellRequestFinished(Context);
	}
}

void ACSKGameState::HandleQuickEffectSelectionStart(bool bNullify)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// We only save if nullifying, doing this when not will result in potentially
		// using this saved value when allowing for a post quick effect selection
		/*if (bNullify)
		{
			ensure(TimerState == ECSKTimerState::ActionPhase);
			NewActionPhaseTimeRemaining = TimeRemaining;
		}*/

		ACSKGameMode* GameMode = CastChecked<ACSKGameMode>(AuthorityGameMode);

		// Start timing selection
		int32 SelectTime = GameMode->GetQuickEffectCounterTime();
		ActivateTickTimer(ECSKTimerState::QuickEffect, SelectTime);

		Multi_HandleQuickEffectSelection(bNullify);
	}
}

void ACSKGameState::HandleBonusSpellSelectionStart()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		ACSKGameMode* GameMode = CastChecked<ACSKGameMode>(AuthorityGameMode);

		// Start timing selection
		int32 SelectTime = GameMode->GetBonusSpellSelectTime();
		ActivateTickTimer(ECSKTimerState::BonusSpell, SelectTime);

		Multi_HandleBonusSpellSelection();
	}
}

bool ACSKGameState::HasPlayerMovedRequiredTiles(const ACSKPlayerController* Controller) const
{
	ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
	if (PlayerState)
	{
		return PlayerState->GetTilesTraversedThisRound() >= MinTileMovements;
	}

	return false;
}

int32 ACSKGameState::GetPlayersNumRemainingMoves(const ACSKPlayerState* PlayerState) const
{
	if (PlayerState)
	{
		int32 TilesTraversed = PlayerState->GetTilesTraversedThisRound();
		int32 BonusTiles = PlayerState->GetBonusTileMovements();

		// The max amount of movements a player can make during the move action. Bonus tiles
		// can be negative (to signal less moves) but should ultimately be clamped to not exceed min
		int32 CalculatedMaxMovements = FMath::Max(MinTileMovements, MaxTileMovements + BonusTiles);

		if (TilesTraversed < CalculatedMaxMovements)
		{
			return CalculatedMaxMovements - TilesTraversed;
		}
	}

	return 0;
}

bool ACSKGameState::GetTilesPlayerCanMoveTo(const ACSKPlayerController* Controller, TArray<ATile*>& OutTiles, bool bPathfind) const
{
	OutTiles.Reset();

	if (!BoardManager)
	{
		return false;
	}

	const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
	if (PlayerState)
	{
		ACastle* CastlePawn = PlayerState->GetCastle();

		// Player might not be able to move anymore this round
		int32 MaxDistance = GetPlayersNumRemainingMoves(PlayerState);
		if (MaxDistance > 0)
		{
			TArray<ATile*> Candidates;
			if (BoardManager->GetTilesWithinDistance(CastlePawn->GetCachedTile(), MaxDistance, Candidates))
			{
				if (bPathfind)
				{
					SCOPE_CYCLE_COUNTER(STAT_CSKGameStateGetTilesPlayerCanMoveToPathfind);
					
					// Initialize this here to avoid creation every loop
					FBoardPath BoardPath;

					for (ATile* Tile : Candidates)
					{
						if (BoardManager->FindPath(CastlePawn->GetCachedTile(), Tile, BoardPath, false, MaxDistance))
						{
							OutTiles.Add(Tile);
						}
					}

					return OutTiles.Num() > 0;
				}
				else
				{
					// Not all these tiles may be reachable in given moves (player might need to walk
					// around an obstacle) but they are within MaxDistance tiles of eachother 
					// This is usefull for small move ranges (eg 1 or 2 tiles max)
					OutTiles = MoveTemp(Candidates);
					return true;
				}
			}
		}
	}

	return false;
}

bool ACSKGameState::GetTilesPlayerCanBuildOn(const ACSKPlayerController* Controller, TArray<ATile*>& OutTiles)
{
	OutTiles.Reset();

	if (!BoardManager)
	{
		return false;
	}

	const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
	if (PlayerState)
	{
		ACastle* CastlePawn = PlayerState->GetCastle();
		if (BoardManager->GetTilesWithinDistance(CastlePawn->GetCachedTile(), MaxBuildRange, OutTiles))
		{
			// We need to remove any portal tiles
			OutTiles.RemoveAll([this](const ATile* Tile)->bool
			{
				return !BoardManager->IsPlayerPortalTile(Tile);
			});
		}
	}

	return OutTiles.Num() > 0;
}

bool ACSKGameState::CanPlayerBuildTower(const ACSKPlayerController* Controller, TSubclassOf<UTowerConstructionData> TowerTemplate) const
{
	const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
	if (PlayerState)
	{
		return CanPlayerBuildTower(PlayerState, TowerTemplate);
	}

	return false;
}

bool ACSKGameState::CanPlayerBuildMoreTowers(const ACSKPlayerController* Controller) const
{
	// No towers might be in this match
	if (AvailableTowers.Num() > 0)
	{
		const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
		if (PlayerState)
		{
			for (TSubclassOf<UTowerConstructionData> TowerTemplate : AvailableTowers)
			{
				if (CanPlayerBuildTower(PlayerState, TowerTemplate))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool ACSKGameState::GetTowersPlayerCanBuild(const ACSKPlayerController* Controller, TArray<TSubclassOf<UTowerConstructionData>>& OutTowers) const
{
	OutTowers.Reset();

	// No towers might be in this match
	if (AvailableTowers.Num() > 0)
	{
		const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
		if (PlayerState)
		{
			for (TSubclassOf<UTowerConstructionData> TowerTemplate : AvailableTowers)
			{
				if (CanPlayerBuildTower(PlayerState, TowerTemplate))
				{
					OutTowers.Add(TowerTemplate);
				}
			}
		}
	}

	return OutTowers.Num() > 0;
}

int32 ACSKGameState::GetPlayerNumRemainingSpellCasts(const ACSKPlayerState* PlayerState, bool& bOutInfinite) const
{
	bOutInfinite = false;

	if (PlayerState)
	{
		int32 SpellsCast = PlayerState->GetSpellsCastThisRound();
		int32 TotalSpellCasts = PlayerState->GetMaxNumSpellUses();

		if (PlayerState->HasInfiniteSpellUses())
		{
			bOutInfinite = true;
			return 1;
		}
		else
		{
			return FMath::Max(0, TotalSpellCasts - SpellsCast);
		}
	}

	return 0;
}

bool ACSKGameState::CanPlayerCastSpell(const ACSKPlayerController* Controller, ATile* TargetTile, 
	TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, int32 AdditionalMana) const
{
	if (!SpellCard)
	{
		return false;
	}

	const USpellCard* DefaultSpellCard = SpellCard.GetDefaultObject();

	const ACSKPlayerState* PlayerState = Controller ? Controller->GetCSKPlayerState() : nullptr;
	if (PlayerState)
	{
		TSubclassOf<USpell> Spell = DefaultSpellCard->GetSpellAtIndex(SpellIndex);
		if (!Spell)
		{
			return false;
		}

		const USpell* DefaultSpell = Spell.GetDefaultObject();

		// This spell might not accept the tile as a target
		if (!DefaultSpell->CanActivateSpell(PlayerState, TargetTile))
		{
			return false;
		}

		// If we can afford the static cost of this spell.
		// This discount only gets applied to this cost
		int32 DiscountedMana = 0;
		if (PlayerState->GetDiscountedManaIfAffordable(DefaultSpell->GetSpellStaticCost(), DiscountedMana))
		{
			int32 FinalCost = DefaultSpell->CalculateFinalCost(PlayerState, TargetTile, DiscountedMana, AdditionalMana);
			return PlayerState->HasRequiredMana(FinalCost);
		}
	}

	return false;
}

void ACSKGameState::UpdateRules()
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		ActionPhaseTime = GameMode->GetActionPhaseTime();
		MaxNumTowers = GameMode->GetMaxNumTowers();
		MaxNumDuplicatedTowers = GameMode->GetMaxNumDuplicatedTowers();
		MaxNumDuplicatedTowerTypes = GameMode->GetMaxNumDuplicatedTowerTypes();
		MaxNumLegendaryTowers = GameMode->GetMaxNumLegendaryTowers();
		MaxBuildRange = GameMode->GetMaxBuildRange();
		MinTileMovements = GameMode->GetMinTileMovementsPerTurn();
		MaxTileMovements = GameMode->GetMaxTileMovementsPerTurn();

		AvailableTowers = GameMode->GetAvailableTowers();
		
		// Zero means indefinite
		if (ActionPhaseTime == 0)
		{
			ActionPhaseTime = -1;
		}

		UE_LOG(LogConquest, Log, TEXT("ACSKGameState: Rules updated"));
	}
}

bool ACSKGameState::CanPlayerBuildTower(const ACSKPlayerState* PlayerState, TSubclassOf<UTowerConstructionData> TowerTemplate) const
{
	UTowerConstructionData* ConstructData = TowerTemplate.GetDefaultObject();
	if (!ConstructData)
	{
		return false;
	}

	if (!ConstructData->TowerClass)
	{
		UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::CanPlayerBuildTower: Tower class for construction data %s is invalid."), *ConstructData->GetName());
		return false;
	}

	// Is tower to expensive? We do not apply discount as it only applies to spells
	if (!PlayerState->HasRequiredGold(ConstructData->GoldCost) || !PlayerState->HasRequiredMana(ConstructData->ManaCost, false))
	{
		return false;
	}

	TSubclassOf<ATower> TowerClass = ConstructData->TowerClass;
	ATower* DefaultTower = TowerClass.GetDefaultObject();
	check(DefaultTower);

	// Different cases depending on if tower is legendary
	if (DefaultTower->IsLegendaryTower())
	{
		// Has player built max amount of legendary towers allowed?
		if (MaxNumLegendaryTowers > 0 && PlayerState->GetNumLegendaryTowersOwned() >= MaxNumLegendaryTowers)
		{
			return false;
		}

		// There can only be one instance 
		if (GetTowerInstanceCount(TowerClass) > 0)
		{
			return false;
		}
	}
	else
	{
		// Has player built the max amount of normal towers allowed?
		if (MaxNumTowers > 0 && PlayerState->GetNumNormalTowersOwned() >= MaxNumTowers)
		{
			return false;
		}

		int32 TowerInstanceCount = PlayerState->GetNumOwnedTowerDuplicates(TowerClass);

		// Has player already built the max amount of duplicates for this tower?
		if (MaxNumDuplicatedTowers > 0 && TowerInstanceCount >= MaxNumDuplicatedTowers)
		{
			return false;
		}

		// Has player already created too many duplicates for different towers?
		if (MaxNumDuplicatedTowerTypes > 0 && PlayerState->GetNumOwnedTowerDuplicateTypes() >= MaxNumDuplicatedTowerTypes)
		{
			// Player might be attempting to build duplicate of this building
			if ((TowerInstanceCount + 1) > 1)
			{
				return false;
			}
		}
	}

	// If this point has been reached, it means the player is allowed to build this tower
	return true;
}

void ACSKGameState::Multi_HandleMoveRequestConfirmed_Implementation()
{
	
}

void ACSKGameState::Multi_HandleMoveRequestFinished_Implementation()
{
	
}

void ACSKGameState::Multi_HandleBuildRequestConfirmed_Implementation(ATile* TargetTile)
{
	
}

void ACSKGameState::Multi_HandleBuildRequestFinished_Implementation(ATower* NewTower)
{
	if (ensure(NewTower))
	{
		// Update tower instance counters
		TSubclassOf<ATower> TowerClass = NewTower->GetClass();
		if (TowerInstanceTable.Contains(TowerClass))
		{
			TowerInstanceTable[TowerClass]++;
		}
		else
		{
			TowerInstanceTable.Add(TowerClass, 1);
		}
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("ACSKGameState::Multi_HandleBuildRequestFinished: NewTower is null"));
	}
}

void ACSKGameState::Multi_HandleSpellRequestConfirmed_Implementation(EActiveSpellContext Context, ATile* TargetTile)
{

}

void ACSKGameState::Multi_HandleSpellRequestFinished_Implementation(EActiveSpellContext Context)
{

}

void ACSKGameState::Multi_HandleQuickEffectSelection_Implementation(bool bNullify)
{

}

void ACSKGameState::Multi_HandleBonusSpellSelection_Implementation()
{

}

void ACSKGameState::HandlePortalReached(ACSKPlayerController* Controller, ATile* ReachedPortal)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		Multi_HandlePortalReached(Controller->GetCSKPlayerState(), ReachedPortal);
	}
}

void ACSKGameState::HandleCastleDestroyed(ACSKPlayerController* Controller, ACastle* DestroyedCastle)
{
	// Castles can be destroyed during the action phase (via Spells) or the end round action phase (via Towers)
	if (IsActionPhaseActive() || IsEndRoundPhaseActive() && HasAuthority())
	{
		// We want to freeze the timer to prevent timer events from being sent
		// (There is a chance that custom timer is currently active)
		bTimerPaused = true;

		Multi_HandleCastleDestroyed(Controller->GetCSKPlayerState(), DestroyedCastle);
	}
}

void ACSKGameState::Multi_HandlePortalReached_Implementation(ACSKPlayerState* Player, ATile* ReachedPortal)
{
}

void ACSKGameState::Multi_HandleCastleDestroyed_Implementation(ACSKPlayerState* Player, ACastle* DestroyedCastle)
{
}

float ACSKGameState::GetMatchTimeSeconds() const
{
	if (MatchState == ECSKMatchState::CoinFlip || MatchState == ECSKMatchState::Running)
	{
		float MatchCurrentTime = GetWorld()->GetTimeSeconds();
		return MatchCurrentTime - MatchStartTime;
	}
	else if (MatchState == ECSKMatchState::WaitingPostMatch)
	{
		return MatchEndTime - MatchStartTime;
	}

	return 0.f;
}
