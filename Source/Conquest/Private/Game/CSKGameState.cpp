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
	ActionPhaseTimeRemaining = -1.f;
	bFreezeActionPhaseTimer = false;
	bCountdownQuickEffectTimer = false;

	ActionPhaseTime = 90.f;
	MaxNumTowers = 7;
	MaxNumDuplicatedTowers = 2;
	MaxNumLegendaryTowers = 1;
	MinTileMovements = 1;
	MaxTileMovements = 2;

	RoundsPlayed = 0;
}

void ACSKGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ShouldCountdownPhaseTimer())
	{
		// Bonus spell selection
		if (bCountdownBonusSpellTimer)
		{
			if (!IsBonusSpellCounterTimed())
			{
				return;
			}

			BonusSpellCounterTimerRemaining = FMath::Max(0.f, BonusSpellCounterTimerRemaining - DeltaTime);
			if (BonusSpellCounterTimerRemaining == 0.f)
			{
				// Game mode only exists on the server
				ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
				if (GameMode)
				{
					// Default to player skipping to use spell
					GameMode->RequestSkipBonusSpell();
					bCountdownBonusSpellTimer = false;
				}
			}
		}
		// Quick effect selection
		else if (bCountdownQuickEffectTimer)
		{
			if (!IsQuickEffectCounterTimed())
			{
				return;
			}

			QuickEffectCounterTimeRemaining = FMath::Max(0.f, QuickEffectCounterTimeRemaining - DeltaTime);
			if (QuickEffectCounterTimeRemaining == 0.f)
			{
				// Game mode only exists on the server
				ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
				if (GameMode)
				{
					// Default to player skipping to counter
					GameMode->RequestSkipQuickEffect();
					bCountdownQuickEffectTimer = false;
				}
			}
		}
		// Action phase timer
		else if (IsActionPhaseTimed())
		{
			ActionPhaseTimeRemaining = FMath::Max(0.f, ActionPhaseTimeRemaining - DeltaTime);
			if (HasAuthority() && ActionPhaseTimeRemaining == 0.f)
			{
				// Game mode only exists on the server
				ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
				if (GameMode)
				{
					// TODO: notify game mode that time has run out
					// Game mode should then auto path closer to opponents portal (if player hasn't moved)
					GameMode->RequestEndActionPhase(true);
				}
			}
		}
	}
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
	DOREPLIFETIME(ACSKGameState, ActionPhaseTimeRemaining);
	DOREPLIFETIME(ACSKGameState, QuickEffectCounterTimeRemaining);
	DOREPLIFETIME(ACSKGameState, BonusSpellCounterTimerRemaining);

	DOREPLIFETIME(ACSKGameState, ActionPhaseTime);
	DOREPLIFETIME(ACSKGameState, MaxNumTowers);
	DOREPLIFETIME(ACSKGameState, MaxNumDuplicatedTowers);
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
	if (HasAuthority() && !LocalPlayerPawn)
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
	
	// Only save on first round
	if (RoundsPlayed == 1)
	{
		// Game mode only exists on the server
		ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			CoinTossWinnerPlayerID = GameMode->GetStartingPlayersID();
		}
	}
}

void ACSKGameState::NotifyFirstActionPhaseStart()
{
	++RoundsPlayed;
	ResetActionPhaseProperties();
}

void ACSKGameState::NotifySecondActionPhaseStart()
{
	ResetActionPhaseProperties();
}

void ACSKGameState::NotifyEndRoundPhaseStart()
{
	ResetActionPhaseProperties();
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

int32 ACSKGameState::GetTowerInstanceCount(TSubclassOf<ATower> Tower) const
{
	const int32* Num = TowerInstanceTable.Find(Tower);
	if (Num != nullptr)
	{
		return *Num;
	}

	return 0;
}

float ACSKGameState::GetCountdownTimeRemaining(bool& bOutIsInfinite) const
{
	bOutIsInfinite = false;

	if (bCountdownBonusSpellTimer)
	{
		if (IsBonusSpellCounterTimed())
		{
			return BonusSpellCounterTimerRemaining;
		}
		else
		{
			bOutIsInfinite = true;
		}
	}
	else if (bCountdownQuickEffectTimer)
	{
		if (IsQuickEffectCounterTimed())
		{
			return QuickEffectCounterTimeRemaining;
		}
		else
		{
			bOutIsInfinite = true;
		}
	}
	else if (IsActionPhaseActive())
	{
		if (IsActionPhaseTimed())
		{
			return ActionPhaseTimeRemaining;
		}
		else
		{
			bOutIsInfinite = true;
		}
	}

	return 0.f;
}

void ACSKGameState::AddBonusActionPhaseTime()
{
	if (IsActionPhaseTimed())
	{
		// Game mode only exists on the server
		ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			ActionPhaseTimeRemaining = FMath::Min(GameMode->GetActionPhaseTime(), ActionPhaseTimeRemaining + GameMode->GetBonusActionPhaseTime());
		}
	}
}

void ACSKGameState::ResetActionPhaseProperties()
{
	ActionPhaseTimeRemaining = ActionPhaseTime;
	QuickEffectCounterTimeRemaining = -1.f;

	// This will disable tick when entering end round phase
	SetFreezeActionPhaseTimer(!IsActionPhaseActive());

	// Determine which players action phase it is
	if (IsActionPhaseActive())
	{
		ActionPhasePlayerID = RoundState == ECSKRoundState::FirstActionPhase ? CoinTossWinnerPlayerID : FMath::Abs(CoinTossWinnerPlayerID - 1);
	}
	else
	{
		ActionPhasePlayerID = -1;
	}
}

void ACSKGameState::HandleMoveRequestConfirmed()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		Multi_HandleMoveRequestConfirmed();
	}
}

void ACSKGameState::HandleMoveRequestFinished()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		AddBonusActionPhaseTime();

		Multi_HandleMoveRequestFinished();
	}
}

void ACSKGameState::HandleBuildRequestConfirmed(ATile* TargetTile)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		Multi_HandleBuildRequestConfirmed(TargetTile);
	}
}

void ACSKGameState::HandleBuildRequestFinished(ATower* NewTower)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		AddBonusActionPhaseTime();

		Multi_HandleBuildRequestFinished(NewTower);
	}
}

void ACSKGameState::HandleSpellRequestConfirmed(ATile* TargetTile)
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		Multi_HandleSpellRequestConfirmed(TargetTile);
	}
}

void ACSKGameState::HandleSpellRequestFinished()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		// Add bonus time after an action is complete
		AddBonusActionPhaseTime();

		Multi_HandleSpellRequestFinished();
	}
}

void ACSKGameState::HandleQuickEffectSelectionStart()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		ACSKGameMode* GameMode = CastChecked<ACSKGameMode>(AuthorityGameMode);

		// Zero means indefinite
		QuickEffectCounterTimeRemaining = GameMode->GetQuickEffectCounterTime();
		if (QuickEffectCounterTimeRemaining == 0.f)
		{
			QuickEffectCounterTimeRemaining = -1.f;
		}

		Multi_HandleQuickEffectSelection();
	}
}

void ACSKGameState::HandleBonusSpellSelectionStart()
{
	if (IsActionPhaseActive() && HasAuthority())
	{
		ACSKGameMode* GameMode = CastChecked<ACSKGameMode>(AuthorityGameMode);

		// Zero means indefinite
		BonusSpellCounterTimerRemaining = GameMode->GetBonusSpellSelectTime();
		if (BonusSpellCounterTimerRemaining == 0.f)
		{
			BonusSpellCounterTimerRemaining = -1.f;
		}

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

void ACSKGameState::UpdateRules()
{
	// Game mode only exists on the server
	ACSKGameMode* GameMode = Cast<ACSKGameMode>(AuthorityGameMode);
	if (GameMode)
	{
		ActionPhaseTime = GameMode->GetActionPhaseTime();
		MaxNumTowers = GameMode->GetMaxNumTowers();
		MaxNumDuplicatedTowers = GameMode->GetMaxNumDuplicatedTowers();
		MaxNumLegendaryTowers = GameMode->GetMaxNumLegendaryTowers();
		MaxBuildRange = GameMode->GetMaxBuildRange();
		MinTileMovements = GameMode->GetMinTileMovementsPerTurn();
		MaxTileMovements = GameMode->GetMaxTileMovementsPerTurn();

		AvailableTowers = GameMode->GetAvailableTowers();
		
		// Zero means indefinite
		if (ActionPhaseTime == 0.f)
		{
			ActionPhaseTime = -1.f;
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
		if (PlayerState->GetNumLegendaryTowersOwned() >= MaxNumLegendaryTowers)
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
		if (PlayerState->GetNumNormalTowersOwned() >= MaxNumTowers)
		{
			return false;
		}

		// Has player already built the max amount of duplicates?
		if (PlayerState->GetNumOwnedTowerDuplicates(TowerClass) >= MaxNumDuplicatedTowers)
		{
			return false;
		}
	}

	// If this point has been reached, it means the player is allowed to build this tower
	return true;
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

void ACSKGameState::Multi_HandleMoveRequestConfirmed_Implementation()
{
	SetFreezeActionPhaseTimer(true);
}

void ACSKGameState::Multi_HandleMoveRequestFinished_Implementation()
{
	SetFreezeActionPhaseTimer(false);
}

void ACSKGameState::Multi_HandleBuildRequestConfirmed_Implementation(ATile* TargetTile)
{
	SetFreezeActionPhaseTimer(true);
}

void ACSKGameState::Multi_HandleBuildRequestFinished_Implementation(ATower* NewTower)
{
	SetFreezeActionPhaseTimer(false);

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

void ACSKGameState::Multi_HandleSpellRequestConfirmed_Implementation(ATile* TargetTile)
{
	SetFreezeActionPhaseTimer(true);
	bCountdownQuickEffectTimer = false;
	bCountdownBonusSpellTimer = false;
}

void ACSKGameState::Multi_HandleSpellRequestFinished_Implementation()
{
	SetFreezeActionPhaseTimer(false);
	bCountdownBonusSpellTimer = false;
}

void ACSKGameState::Multi_HandleQuickEffectSelection_Implementation()
{
	// We want to count down the quick effect selection time
	bCountdownQuickEffectTimer = true;
	SetFreezeActionPhaseTimer(false);
}

void ACSKGameState::Multi_HandleBonusSpellSelection_Implementation()
{
	// We want to count down the bonus spell selection time
	bCountdownBonusSpellTimer = true;
	SetFreezeActionPhaseTimer(false);
}
