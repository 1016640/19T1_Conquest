// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKGameMode.h"
#include "CSKGameInstance.h"
#include "CSKGameState.h"
#include "CSKHUD.h"
#include "CSKPawn.h"
#include "CSKPlayerController.h"
#include "CSKPlayerStart.h"
#include "CSKPlayerState.h"

#include "BoardManager.h"
#include "BoardPathFollowingComponent.h"
#include "Castle.h"
#include "CastleAIController.h"
#include "CoinSequenceActor.h"
#include "GameDelegates.h"
#include "HealthComponent.h"
#include "Spell.h"
#include "SpellActor.h"
#include "SpellCard.h"
#include "Tile.h"
#include "TimerManager.h"
#include "Tower.h"
#include "TowerConstructionData.h"
#include "WinnerSequenceActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "CSKGameMode"

ACSKGameMode::ACSKGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	DefaultPawnClass = ACSKPawn::StaticClass();
	GameStateClass = ACSKGameState::StaticClass();
	HUDClass = ACSKHUD::StaticClass();
	PlayerControllerClass = ACSKPlayerController::StaticClass();
	PlayerStateClass = ACSKPlayerState::StaticClass();

	DefaultPlayerName = LOCTEXT("DefaultPlayerName", "Sorcerer");
	bUseSeamlessTravel = true;

	Player1CastleClass = ACastle::StaticClass();
	Player2CastleClass = ACastle::StaticClass();
	CastleAIControllerClass = ACastleAIController::StaticClass();

	MatchState = ECSKMatchState::EnteringGame;
	RoundState = ECSKRoundState::Invalid;
	MatchWinner = nullptr;
	MatchWinCondition = ECSKMatchWinCondition::Unknown;
	
	bWinnerSequenceActorSpawned = false;
	bWinnerSequenceOrActionFinished = false;

	StartingGold = 5;
	StartingMana = 3;
	CollectionPhaseGold = 3;
	CollectionPhaseMana = 2;
	MaxGold = 30;
	MaxMana = 30;

	MaxNumTowers = 7;
	MaxNumLegendaryTowers = 1;
	MaxNumDuplicatedTowers = 2;
	MaxNumDuplicatedTowerTypes = 2;
	MaxBuildRange = 4;

	ActionPhaseTime = 90;
	BonusActionPhaseTime = 20;
	MinTileMovements = 1;
	MaxTileMovements = 2;
	bLimitOneMoveActionPerTurn = false;
	MaxSpellUses = 1;
	MaxSpellCardsInHand = 3;

	QuickEffectCounterTime = 15;
	BonusSpellSelectTime = 15;
	InitialMatchDelay = 2.f;
	PostMatchDelay = 15.f;

	PortalReachedSequenceClass = AWinnerSequenceActor::StaticClass();
	CastleDestroyedSequenceClass = AWinnerSequenceActor::StaticClass();

	#if WITH_EDITORONLY_DATA
	P1AssignedColor = FColor::Red;
	P2AssignedColor = FColor::Green;
	#endif
}

void ACSKGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// Fix the size of the players array, as it won't be changing throughout the match
	Players.SetNum(CSK_MAX_NUM_PLAYERS);
	PlayersLeft = 0;

	Super::InitGame(MapName, Options, ErrorMessage);

	// Entering game is default state, we call it here anyways to fire off events
	EnterMatchState(ECSKMatchState::EnteringGame);

	// Callbacks required
	FGameDelegates& GameDelegates = FGameDelegates::Get();
	GameDelegates.GetHandleDisconnectDelegate().AddUObject(this, &ACSKGameMode::OnDisconnect);
}

void ACSKGameMode::InitGameState()
{
	Super::InitGameState();

	// First tell game state to save the board we are using for this match
	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->SetMatchBoardManager(UConquestFunctionLibrary::FindMatchBoardManager(this));
	}
}

void ACSKGameMode::StartPlay()
{
	if (MatchState == ECSKMatchState::EnteringGame)
	{
		EnterMatchState(ECSKMatchState::WaitingPreMatch);
	}

	// We might be able to start immediately
	if (ShouldStartMatch())
	{
		StartMatch();
	}
	else
	{
		// Keep manually checking
		SetActorTickEnabled(true);
	}
}

void ACSKGameMode::Logout(AController* Exiting)
{
	ACSKPlayerController* Controller = Cast<ACSKPlayerController>(Exiting);
	if (Controller && Players.IsValidIndex(Controller->CSKPlayerID))
	{
		// We add one since PlayerID is an index
		//PlayersLeft |= (Controller->CSKPlayerID + 1);
		AbortMatch();
	}

	Super::Logout(Exiting);
}

void ACSKGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// We need to set which player this is before continuing the login
	{
		if (GetPlayer1Controller() != nullptr)
		{
			// We should only ever have two players
			if (!ensure(!GetPlayer2Controller()))
			{
				UE_LOG(LogConquest, Error, TEXT("More than 2 people of joined a CSK match even though only 2 max are allowed"));
			}
			else
			{
				SetPlayerWithID(CastChecked<ACSKPlayerController>(NewPlayer), 1);
			}
		}
		else
		{
			SetPlayerWithID(CastChecked<ACSKPlayerController>(NewPlayer), 0);
		}
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

APawn* ACSKGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	if (Pawn)
	{
		ACSKPlayerController* CSKController = Cast<ACSKPlayerController>(NewPlayer);
		if (CSKController)
		{
			ACastleAIController* CastleController = SpawnDefaultCastleFor(CSKController);
			if (CastleController)
			{
				CSKController->SetCastleController(CastleController);
			}
		}
	}

	return Pawn;
}

AActor* ACSKGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	APlayerStart* PawnSpawn = FindPlayerStartWithTag(ACSKPlayerStart::CSKPawnSpawnTag);
	if (PawnSpawn)
	{
		return PawnSpawn;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

bool ACSKGameMode::HasMatchStarted() const
{
	return !(MatchState == ECSKMatchState::EnteringGame || MatchState == ECSKMatchState::WaitingPreMatch);
}

#if WITH_EDITOR
void ACSKGameMode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACSKGameMode, MinTileMovements))
	{
		MaxTileMovements = FMath::Max(MinTileMovements, MaxTileMovements);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACSKGameMode, MaxTileMovements))
	{
		MinTileMovements = FMath::Min(MinTileMovements, MaxTileMovements);
	}
}
#endif

bool ACSKGameMode::IsMatchValid() const
{
	if (HasMatchStarted())
	{
		return (Players[0] != nullptr && Players[1] != nullptr);
	}

	return true;
}

ACastleAIController* ACSKGameMode::SpawnDefaultCastleFor(ACSKPlayerController* NewPlayer) const
{
	int32 PlayerID = NewPlayer ? NewPlayer->CSKPlayerID : -1;
	if (PlayerID != -1)
	{
		TSubclassOf<ACastle> CastleTemplate = PlayerID == 0 ? Player1CastleClass : Player2CastleClass;
		if (CastleTemplate)
		{
			ACastle* Castle = SpawnCastleAtPortal(NewPlayer, CastleTemplate);
			if (Castle)
			{
				return SpawnCastleControllerFor(Castle);
			}
		}
		else
		{
			UE_LOG(LogConquest, Error, TEXT("Castle Class for Player %i was invalid"), PlayerID + 1);
		}
	}

	return nullptr;
}

ACastle* ACSKGameMode::SpawnCastleAtPortal(ACSKPlayerController* Controller, TSubclassOf<ACastle> Class) const
{
	check(Controller);
	check(Class);

	const ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		ATile* PortalTile = BoardManager->GetPlayerPortalTile(Controller->CSKPlayerID);
		if (PortalTile)
		{
			// Remove scale from transform
			FTransform TileTransform = PortalTile->GetTransform();
			TileTransform.SetScale3D(FVector::OneVector);

			FActorSpawnParameters SpawnParams;
			SpawnParams.ObjectFlags |= RF_Transient;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.bDeferConstruction = true;

			ACastle* Castle = GetWorld()->SpawnActor<ACastle>(Class, TileTransform, SpawnParams);
			if (Castle)
			{
				// Initialize castle with the players state before spawning, so it gets sent with initial replication
				Castle->SetBoardPieceOwnerPlayerState(Controller->GetCSKPlayerState());
				Castle->FinishSpawning(TileTransform);

				UHealthComponent* HealthComp = Castle->GetHealthComponent();
				check(HealthComp)

				// Listen out for when this castle takes damage
				HealthComp->OnHealthChanged.AddDynamic(this, &ACSKGameMode::OnBoardPieceHealthChanged);
			}
			else
			{
				UE_LOG(LogConquest, Warning, TEXT("Failed to spawn castle for Player %i"), Controller->CSKPlayerID + 1);
			}

			return Castle;
		}
	}

	return nullptr;
}

ACastleAIController* ACSKGameMode::SpawnCastleControllerFor(ACastle* Castle) const
{
	check(Castle);

	// The castle might have already spawned with a controller
	if (Castle->GetController() != nullptr)
	{
		AController* ExistingController = Castle->GetController();
		if (ExistingController->IsA<ACastleAIController>())
		{
			// Static cast would work here as well
			return Cast<ACastleAIController>(ExistingController);
		}
		else
		{
			ExistingController->UnPossess();
			ExistingController->Destroy();
		}
	}

	// Get a valid controller to use (we should never be null)
	TSubclassOf<ACastleAIController> ControllerTemplate = CastleAIControllerClass;
	if (!ControllerTemplate)
	{
		ACastle* DefaultCastle = Castle->StaticClass()->GetDefaultObject<ACastle>();
		ControllerTemplate = DefaultCastle->AIControllerClass ? DefaultCastle->AIControllerClass : ACastleAIController::StaticClass();
	}

	check(ControllerTemplate);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = Castle->Instigator;
	SpawnParams.OverrideLevel = Castle->GetLevel();
	SpawnParams.ObjectFlags = RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACastleAIController* NewController = GetWorld()->SpawnActor<ACastleAIController>(ControllerTemplate, Castle->GetActorLocation(), Castle->GetActorRotation(), SpawnParams);
	if (NewController)
	{
		NewController->Possess(Castle);
	}

	return NewController;
}

APlayerStart* ACSKGameMode::FindPlayerStartWithTag(const FName& InTag) const
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			if (It->PlayerStartTag == InTag)
			{
				return *It;
			}
		}
	}

	return nullptr;
}

ACSKPlayerController* ACSKGameMode::GetOpposingPlayersController(int32 PlayerID) const
{
	if (PlayerID == 0 || PlayerID == 1)
	{
		return Players[FMath::Abs(PlayerID - 1)];
	}

	return nullptr;
}

void ACSKGameMode::SetPlayerWithID(ACSKPlayerController* Controller, int32 PlayerID)
{
	if (!Controller || Controller->CSKPlayerID >= 0)
	{
		return;
	}

	if (Players.IsValidIndex(PlayerID))
	{
		// Space might already be occupied
		if (Players[PlayerID] == nullptr)
		{
			// Save reference to this player
			Players[PlayerID] = Controller;
			Controller->CSKPlayerID = PlayerID;

			ACSKPlayerState* PlayerState = Controller->GetCSKPlayerState();
			if (PlayerState)
			{
				PlayerState->SetCSKPlayerID(PlayerID);

				#if WITH_EDITORONLY_DATA
				PlayerState->SetAssignedColor(PlayerID == 0 ? P1AssignedColor : P2AssignedColor);
				#endif
			}
		}
	}
}

void ACSKGameMode::EnterMatchState(ECSKMatchState NewState)
{
	if (NewState != MatchState)
	{
		if (Handle_EnterMatchState.IsValid())
		{
			// A new state might be delayed with this call interrupting it,
			// we are disabling it to avoid rapid succession of state changes
			FTimerManager& TimerManager = GetWorldTimerManager();
			TimerManager.ClearTimer(Handle_EnterMatchState);

			Handle_EnterMatchState.Invalidate();

			// I wanted to add a log check here to inform developers if this function was
			// called while a delay change was pending, but FTimerManager doesn't offer the
			// ability to get a timers status, so .IsTimerActive() would return true even
			// if the this function was being executed by the timer manager.

			// TODO?
			// Another option would be to make another function that the timer is set to call instead,
			// where the timer is disabled before calling this function. This would result in the
			// timer only be active in this function if a delayed change is pending.
		}

		static UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECSKMatchState"));
		if (EnumClass)
		{
			UE_LOG(LogConquest, Log, TEXT("Entering Match State: %s"), *EnumClass->GetNameByIndex((int32)NewState).ToString());
		}
	
		ECSKMatchState OldState = MatchState;
		MatchState = NewState;

		// Handle any changes required due to new state
		HandleMatchStateChange(OldState, NewState);

		// Inform clients of change
		ACSKGameState* CSKGameState = GetGameState<ACSKGameState>();
		if (CSKGameState)
		{
			CSKGameState->SetMatchState(NewState);
		}
	}
}

void ACSKGameMode::EnterRoundState(ECSKRoundState NewState)
{
	// Only enter new states once we have begun the game
	if (NewState != RoundState && IsMatchInProgress())
	{
		if (Handle_EnterRoundState.IsValid())
		{
			// A new state might be delayed with this call interrupting it,
			// we are disabling it to avoid rapid succession of state changes
			FTimerManager& TimerManager = GetWorldTimerManager();
			TimerManager.ClearTimer(Handle_EnterRoundState);

			Handle_EnterRoundState.Invalidate();

			// I wanted to add a log check here to inform developers if this function was
			// called while a delay change was pending, but FTimerManager doesn't offer the
			// ability to get a timers status, so .IsTimerActive() would return true even
			// if the this function was being executed by the timer manager.

			// TODO?
			// Another option would be to make another function that the timer is set to call instead,
			// where the timer is disabled before calling this function. This would result in the
			// timer only be active in this function if a delayed change is pending.
		}

		static UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECSKRoundState"));
		if (EnumClass)
		{
			UE_LOG(LogConquest, Log, TEXT("Entering Round State: %s"), *EnumClass->GetNameByIndex((int32)NewState).ToString());
		}

		ECSKRoundState OldState = RoundState;
		RoundState = NewState;

		// Handle any changes required due to new state
		HandleRoundStateChange(OldState, NewState);

		// Inform clients of change
		ACSKGameState* CSKGameState = GetGameState<ACSKGameState>();
		if (CSKGameState)
		{
			CSKGameState->SetRoundState(NewState);
		}
	}
}

void ACSKGameMode::StartMatch()
{
	if (ShouldStartMatch())
	{
		// We start the match by going to the coin flip, this
		// will also handle skipping the flip sequence if needed
		EnterMatchStateAfterDelay(ECSKMatchState::CoinFlip, 2.f);

		// This could possibly be called from TryStartMatch
		FTimerManager& TimerManager = GetWorldTimerManager();
		TimerManager.ClearTimer(Handle_TryStartMatch);
	}
}

void ACSKGameMode::EndMatch(ACSKPlayerController* WinningPlayer, ECSKMatchWinCondition MetCondition)
{
	if (ShouldEndMatch())
	{
		MatchWinner = WinningPlayer;
		MatchWinCondition = WinningPlayer ? MetCondition : ECSKMatchWinCondition::Unknown;

		// End the match
		EnterMatchState(ECSKMatchState::WaitingPostMatch);
	}
}

void ACSKGameMode::AbortMatch()
{
	// Abandon the match
	EnterMatchState(ECSKMatchState::Aborted);
}

void ACSKGameMode::CacheWinnerAndPlayWinnerSequence(ACSKPlayerController* WinningPlayer, ECSKMatchWinCondition MetCondition)
{
	if (ShouldEndMatch() && WinningPlayer)
	{
		MatchWinner = WinningPlayer;
		MatchWinCondition = MetCondition;

		ACSKPlayerState* PlayerState = WinningPlayer->GetCSKPlayerState();

		// Try to spawn the finish sequence actor, if we fail, just end the match now
		AWinnerSequenceActor* SequenceActor = SpawnWinnerSequenceActor(PlayerState, MetCondition);
		if (SequenceActor)
		{
			bWinnerSequenceActorSpawned = true;
			SequenceActor->OnSequenceFinished.BindDynamic(this, &ACSKGameMode::OnWinnerSequenceFinished);
		}
		else
		{
			// End the match now
			EnterMatchState(ECSKMatchState::WaitingPostMatch);
		}
	}
}

void ACSKGameMode::OnWinnerSequenceFinished()
{
	// The sequence actor could potentially finish before the current action. We want
	// to wait for both the action and the sequence to finish before ending the match
	PostActionCheckEndMatch();
}

bool ACSKGameMode::PostActionCheckEndMatch()
{
	if (bWinnerSequenceActorSpawned)
	{
		if (bWinnerSequenceOrActionFinished)
		{
			// We can now end the match
			EnterMatchState(ECSKMatchState::WaitingPostMatch);
		}
		else
		{
			// We wait for the next time this function is called before ending the match
			bWinnerSequenceOrActionFinished = true;
		}

		return true;
	}

	return false;
}

bool ACSKGameMode::ShouldStartMatch_Implementation() const
{
	// The match has already started
	if (IsMatchInProgress() || HasMatchFinished())
	{
		return false;
	}

	// Both players need to be ready
	for (ACSKPlayerController* Controller : Players)
	{
		if (!Controller || !Controller->HasClientLoadedCurrentWorld())
		{
			return false;
		}
	}

	return true;
}

bool ACSKGameMode::ShouldEndMatch_Implementation() const
{
	return IsMatchInProgress();
}

bool ACSKGameMode::IsMatchInProgress() const
{
	return MatchState == ECSKMatchState::Running;
}

bool ACSKGameMode::HasMatchFinished() const
{
	return 
		MatchState == ECSKMatchState::WaitingPostMatch	||
		MatchState == ECSKMatchState::LeavingGame		||
		MatchState == ECSKMatchState::Aborted;
}

bool ACSKGameMode::IsCollectionPhaseInProgress() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::CollectionPhase;
	}

	return false;
}

bool ACSKGameMode::IsActionPhaseInProgress() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::FirstActionPhase || RoundState == ECSKRoundState::SecondActionPhase;
	}

	return false;
}

bool ACSKGameMode::IsEndRoundPhaseInProgress() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::EndRoundPhase;
	}

	return false;
}

void ACSKGameMode::OnStartWaitingPreMatch()
{
	// Allow actors in the world to start ticking while we wait
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay(); 

	// Keep checking for if we can start the match
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_TryStartMatch, this, &ACSKGameMode::TryStartMatch, 1.f, true, InitialMatchDelay);
}

void ACSKGameMode::OnCoinFlipStart()
{
	// We want to make sure these are done before we start any match operations (including the coin flip)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController && (PlayerController->GetPawn() == nullptr) && PlayerCanRestart(PlayerController))
			{
				RestartPlayer(PlayerController);
			}
		}

		// Wait for level streaming to finish loading
		GEngine->BlockTillLevelStreamingCompleted(GetWorld());
	}

	// We can place the players castles and tile colors here
	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		ABoardManager* BoardManager = CSKGameState->GetBoardManager();
		if (BoardManager)
		{
			for (int32 i = 0; i < CSK_MAX_NUM_PLAYERS; ++i)
			{
				ACSKPlayerController* Controller = Players[i];
				if (!Controller)
				{ 
					continue;
				}

				// We first need to create the player highlight materials for each player
				ACSKPlayerState* PlayerState = Controller->GetCSKPlayerState();
				if (PlayerState)
				{
					BoardManager->SetHighlightColorForPlayer(i, PlayerState->GetAssignedColor());
				}

				// We can now set the players castle on the board, so it refreshes to correct material
				ATile* PortalTile = BoardManager->GetPlayerPortalTile(Controller->CSKPlayerID);
				if (PortalTile)
				{
					BoardManager->PlaceBoardPieceOnTile(Controller->GetCastlePawn(), PortalTile);
				}
				else
				{
					UE_LOG(LogConquest, Error, TEXT("No Portal Tile exists for Player %i! "
						"Unable to place players castle"), Controller->CSKPlayerID + 1);
				}
			}
		}
	}

	PlayersAtCoinSequence = 0;
	bExecutingCoinSequnce = false;

	// We need to find a sequence actor to use
	CoinSequenceActor = UConquestFunctionLibrary::FindCoinSequenceActor(this);

	if (CoinSequenceActor && CoinSequenceActor->CanActivateCoinSequence())
	{
		// We can have sequence setup while players transition to the board
		CoinSequenceActor->SetupCoinSequence();

		// Notify players to transition to board (Who will report back to us once done)
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_TransitionToCoinSequence(CoinSequenceActor);
			}
		}
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("Failed to start coin flip sequence. Skipping the sequence and starting match in 2 seconds"));

		if (CoinSequenceActor)
		{
			CoinSequenceActor->CleanupCoinSequence();
		}

		// Force match to start
		StartingPlayerID = GenerateCoinFlipWinner() ? 0 : 1;
		EnterMatchStateAfterDelay(ECSKMatchState::Running, 2.f);
	}
}

void ACSKGameMode::OnMatchStart()
{
	SetActorTickEnabled(false);

	// Give players the default resources
	ResetResourcesForPlayers();

	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyMatchStarted();

	// Notify each player that match is now starting
	for (ACSKPlayerController* Controller : Players)
	{
		if (Controller)
		{
			Controller->Client_OnMatchStarted();
		}
	}

	// Commence the match
	EnterRoundState(ECSKRoundState::CollectionPhase);
}

void ACSKGameMode::OnMatchFinished()
{
	// Notify each client (let them handle post match screen locally)
	for (ACSKPlayerController* Controller : Players)
	{
		if (Controller)
		{
			Controller->Client_OnMatchFinished(Controller == MatchWinner);
		}
	}

	// Clear any timers we might have active
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearAllTimersForObject(this);

	// Delay exiting so players can read post match states
	EnterMatchStateAfterDelay(ECSKMatchState::LeavingGame, FMath::Max(1.f, PostMatchDelay));
}

void ACSKGameMode::OnFinishedWaitingPostMatch()
{
	#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (World && World->IsPlayInEditor())
	{
		// End PIE session
		ACSKPlayerController* Controller = GetPlayer1Controller();
		if (Controller)
		{
			Controller->ConsoleCommand("Quit", false);
		}

		return;
	}
	#endif

	FString LevelName("L_Lobby");
	TArray<FString> Options{ "listen", "gamemode='Blueprint'/Game/Game/Blueprints/BP_LobbyGameMode.BP_LobbyGameMode'" };

	// Return to the lobby
	UCSKGameInstance::ServerTravelToLevel(this, LevelName, Options);
}

void ACSKGameMode::OnMatchAbort()
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearAllTimersForObject(this);

	OnFinishedWaitingPostMatch();
}

void ACSKGameMode::OnCollectionPhaseStart()
{
	// Clear from previous end round phase
	ClearHealthReports();

	CollectionSequenceFinishedFlags = 0;
	DelayThenStartCollectionPhaseSequence();

	UE_LOG(LogConquest, Log, TEXT("Starting Collection Phase"));
}

void ACSKGameMode::OnFirstActionPhaseStart()
{
	Handle_CollectionSequences.Invalidate();

	UpdateActivePlayerForActionPhase(0);
	check(ActionPhaseActiveController);

	UE_LOG(LogConquest, Log, TEXT("Starting Action Phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);
}

void ACSKGameMode::OnSecondActionPhaseStart()
{
	UpdateActivePlayerForActionPhase(1);
	check(ActionPhaseActiveController);

	UE_LOG(LogConquest, Log, TEXT("Starting Action Phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);
}

void ACSKGameMode::OnEndRoundPhaseStart()
{
	UE_LOG(LogConquest, Log, TEXT("Starting End Round Phase"));

	// Resets action phase active controller
	UpdateActivePlayerForActionPhase(-1);

	// Clear from action phases
	ClearHealthReports();

	EndRoundRunningTower = 0;
	bRunningTowerEndRoundAction = false;
	bInitiatingTowerEndRoundAction = false;

	if (PrepareEndRoundActionTowers())
	{
		// Give the game state some time to replicate round state changes before commencing end round actions
		StartNextEndRoundActionAfterDelay(1.f);
	}
	else
	{
		ACSKGameState* CSKGameState = CastChecked<ACSKGameState>(GameState);
		UE_LOG(LogConquest, Log, TEXT("Skipping End Round Phase for round %i as no placed towers can perform actions"), CSKGameState->GetRound());

		EnterRoundStateAfterDelay(ECSKRoundState::CollectionPhase, 2.f);
	}
}

void ACSKGameMode::TryStartMatch()
{
	if (ShouldStartMatch())
	{
		StartMatch();
	}
}

void ACSKGameMode::EnterMatchStateAfterDelay(ECSKMatchState NewState, float Delay)
{
	if (MatchState == NewState)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	if (TimerManager.IsTimerActive(Handle_EnterMatchState))
	{
		TimerManager.ClearTimer(Handle_EnterMatchState);

		UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::EnterMatchStateAfterDelay: Cancelling existing delay since "
			"a new delay has been requested (This might be called during that delays callback execution though!)."));
	}

	// Create delegate with param state set
	FTimerDelegate DelayedCallback;
	DelayedCallback.BindUObject(this, &ACSKGameMode::EnterMatchState, NewState);

	if (Delay > 0.f)
	{
		TimerManager.SetTimer(Handle_EnterMatchState, DelayedCallback, Delay, false);
	}
	else
	{
		TimerManager.SetTimerForNextTick(DelayedCallback);
		Handle_EnterMatchState.Invalidate();
	}
}

void ACSKGameMode::EnterRoundStateAfterDelay(ECSKRoundState NewState, float Delay)
{
	if (RoundState == NewState)
	{
		return;
	}

	FTimerManager& TimerManager = GetWorldTimerManager();
	if (TimerManager.IsTimerActive(Handle_EnterRoundState))
	{
		TimerManager.ClearTimer(Handle_EnterRoundState);

		UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::EnterRoundStateAfterDelay: Cancelling existing delay since "
			"a new delay has been requested (This might be called during that delays callback execution though!)."));
	}

	// Create delegate with param state set
	FTimerDelegate DelayedCallback;
	DelayedCallback.BindUObject(this, &ACSKGameMode::EnterRoundState, NewState);

	if (Delay > 0.f)
	{
		TimerManager.SetTimer(Handle_EnterRoundState, DelayedCallback, Delay, false);
	}
	else
	{
		TimerManager.SetTimerForNextTick(DelayedCallback);
		Handle_EnterRoundState.Invalidate();
	}
}

void ACSKGameMode::HandleMatchStateChange(ECSKMatchState OldState, ECSKMatchState NewState)
{
	switch (NewState)
	{
		case ECSKMatchState::EnteringGame:
		{
			break;
		}
		case ECSKMatchState::WaitingPreMatch:
		{
			OnStartWaitingPreMatch();
			break;
		}
		case ECSKMatchState::CoinFlip:
		{
			OnCoinFlipStart();
			break;
		}
		case ECSKMatchState::Running:
		{
			OnMatchStart();
			break;
		}
		case ECSKMatchState::WaitingPostMatch:
		{
			OnMatchFinished();
			break;
		}
		case ECSKMatchState::LeavingGame:
		{
			OnFinishedWaitingPostMatch();
			break;
		}
		case ECSKMatchState::Aborted:
		{
			OnMatchAbort();
			break;
		}
	}
}

void ACSKGameMode::HandleRoundStateChange(ECSKRoundState OldState, ECSKRoundState NewState)
{
	switch (NewState)
	{
		case ECSKRoundState::CollectionPhase:
		{
			OnCollectionPhaseStart();
			break;
		}
		case ECSKRoundState::FirstActionPhase:
		{
			OnFirstActionPhaseStart();
			break;
		}
		case ECSKRoundState::SecondActionPhase:
		{
			OnSecondActionPhaseStart();
			break;
		}
		case ECSKRoundState::EndRoundPhase:
		{
			OnEndRoundPhaseStart();
			break;
		}
	}
}

bool ACSKGameMode::CheckSurrenderCondition()
{
	if (!HasMatchFinished())
	{
		// Has a player left?
		if (PlayersLeft != 0)
		{
			// First Bit = Host Left
			// Second Bit = Guest Left
			if (PlayersLeft == 0b01)
			{
				EndMatch(Players[1], ECSKMatchWinCondition::Surrender);
			}
			else if (PlayersLeft == 0b10)
			{
				EndMatch(Players[0], ECSKMatchWinCondition::Surrender);
			}		
			// Both players have left
			else
			{
				AbortMatch();
			}

			return false;
		}
	}

	return true;
}

bool ACSKGameMode::GenerateCoinFlipWinner_Implementation() const
{
	int32 RandomValue = FMath::Rand();
	return (RandomValue % 2) == 1;
}

void ACSKGameMode::OnPlayerTransitionSequenceFinished()
{
	if (MatchState == ECSKMatchState::CoinFlip)
	{
		// We are waiting for both players to transition back to the board
		if (bExecutingCoinSequnce)
		{
			if (PlayersAtCoinSequence > 0)
			{
				--PlayersAtCoinSequence;
				if (PlayersAtCoinSequence == 0)
				{
					CoinSequenceActor->FinishCoinSequence();
					CoinSequenceActor->CleanupCoinSequence();

					bExecutingCoinSequnce = false;

					EnterMatchState(ECSKMatchState::Running);
				}
			}
		}
		// We are waiting for both players to transition to the coin sequence
		else
		{
			// We can't perform the coin flip without the sequence actor
			if (!CoinSequenceActor)
			{
				return;
			}

			// Start the coin flip once both players are ready
			if (PlayersAtCoinSequence < CSK_MAX_NUM_PLAYERS)
			{
				++PlayersAtCoinSequence;
				if (PlayersAtCoinSequence == CSK_MAX_NUM_PLAYERS)
				{
					CoinSequenceActor->StartCoinSequence();
					bExecutingCoinSequnce = true;
				}
			}
		}
	}
}

void ACSKGameMode::OnStartingPlayerDecided(int32 WinningPlayerID)
{
	StartingPlayerID = FMath::Clamp(WinningPlayerID, 0, 1);
	
	// Notify each client of the winner
	for (ACSKPlayerController* Controller : Players)
	{
		if (Controller)
		{
			bool bIsStartingPlayer = Controller->CSKPlayerID == StartingPlayerID;
			Controller->Client_OnStartingPlayerDecided(bIsStartingPlayer);
		}
	}

	// We can wait a bit to give each player some time
	// to read and understand which player is going first
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_PostCoinFlipDelay, this, &ACSKGameMode::PostCoinFlipDelayFinished, 5.f, false);
}

void ACSKGameMode::PostCoinFlipDelayFinished()
{
	for (ACSKPlayerController* Controller : Players)
	{
		if (Controller)
		{
			Controller->Client_TransitionToBoard();
		}
	}
}

ACSKPlayerController* ACSKGameMode::GetActivePlayerForActionPhase(int32 Phase) const
{
	if (IsMatchInProgress())
	{
		// There are two action phases, one for each player.
		if (Players.IsValidIndex(Phase))
		{
			return Players[Phase == 0 ? StartingPlayerID : FMath::Abs(StartingPlayerID - 1)];
		}
	}

	return nullptr;
}

void ACSKGameMode::UpdateActivePlayerForActionPhase(int32 Phase)
{
	// Disable action phase for current active player
	if (ActionPhaseActiveController)
	{
		ActionPhaseActiveController->SetActionPhaseEnabled(false);

		UE_LOG(LogConquest, Log, TEXT("ACSKGameMode: Disabling action phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);
	}

	ActionPhaseActiveController = GetActivePlayerForActionPhase(Phase);
	if (ActionPhaseActiveController)
	{
		ensure(IsActionPhaseInProgress());
		ActionPhaseActiveController->SetActionPhaseEnabled(true);

		UE_LOG(LogConquest, Log, TEXT("ACSKGameMode: Enabling action phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);
	}

	ResetWaitingOnActionFlags();
}

void ACSKGameMode::DelayThenStartCollectionPhaseSequence()
{
	check(IsCollectionPhaseInProgress());

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_CollectionSequences, this, &ACSKGameMode::StartCollectionPhaseSequence, 2.f);
}

void ACSKGameMode::StartCollectionPhaseSequence()
{
	check(IsCollectionPhaseInProgress());

	CollectResourcesForPlayers();

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_CollectionSequences, this, &ACSKGameMode::ForceCollectionPhaseSequenceEnd, 10.f);
}

void ACSKGameMode::ForceCollectionPhaseSequenceEnd()
{
	if (IsCollectionPhaseInProgress())
	{
		EnterRoundState(ECSKRoundState::FirstActionPhase);
	}
}

void ACSKGameMode::ResetResourcesForPlayers()
{
	DeckReshuffleStream.GenerateNewSeed();

	for (int32 i = 0; i < CSK_MAX_NUM_PLAYERS; ++i)
	{
		ResetPlayerResources(Players[i], i + 1);
	}
}

void ACSKGameMode::CollectResourcesForPlayers()
{
	for (int32 i = 0; i < CSK_MAX_NUM_PLAYERS; ++i)
	{
		UpdatePlayerResources(Players[i], i + 1);
	}
}

void ACSKGameMode::ResetPlayerResources(ACSKPlayerController* Controller, int32 PlayerID)
{
	if (ensure(Controller))
	{
		ACSKPlayerState* State = Controller->GetCSKPlayerState();
		if (!ensure(State))
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::ResetPlayerResources: Unable to reset resources for player %i as player state is invalid"), PlayerID);
			return;
		}

		// Default resources
		State->SetResources(StartingGold, StartingMana);

		// Default spell uses
		State->SetSpellUses(MaxSpellUses);
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("ACSKGameMode::ResetPlayerResources: Unable to reset resources for player %i as controller was null!"), PlayerID);
	}
}

void ACSKGameMode::UpdatePlayerResources(ACSKPlayerController* Controller, int32 PlayerID)
{
	if (ensure(Controller))
	{
		ACSKPlayerState* State = Controller->GetCSKPlayerState();
		if (!ensure(State))
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::UpdatePlayerResources: Unable to update resources for player %i as player state is invalid"), PlayerID);
			return;
		}

		int32 GoldToGive = CollectionPhaseGold;
		int32 ManaToGive = CollectionPhaseMana;

		// Cycle through this players towers to collect any additional resources
		TArray<ATower*> PlayersTowers = State->GetOwnedTowers();
		for (ATower* Tower : PlayersTowers)
		{
			if (ensure(Tower) && Tower->WantsCollectionPhaseEvent())
			{
				int32 AdditionalGold = 0;
				int32 AdditionalMana = 0;
				int32 AdditionalSpellUses = 0;

				Tower->BP_GetCollectionPhaseResources(Controller, AdditionalGold, AdditionalMana, AdditionalSpellUses);

				GoldToGive += AdditionalGold;
				ManaToGive += AdditionalMana;
			}
		}

		int32 OriginalGold = State->GetGold();
		int32 OriginalMana = State->GetMana();

		// Update resources
		int32 NewGold = ClampGoldToLimit(OriginalGold + GoldToGive);
		int32 NewMana = ClampManaToLimit(OriginalMana + ManaToGive);
		State->SetResources(NewGold, NewMana);

		// Pick up a spell from the deck (reshuffle the discard pile if required)
		bool bDeckReshuffled = false;
		if (State->NeedsSpellDeckReshuffle())
		{
			State->ResetSpellDeck(AvailableSpellCards, DeckReshuffleStream);
			bDeckReshuffled = true;
		}

		// Player can only carry X amount of cards in hand
		TSubclassOf<USpellCard> SpellCard = nullptr;
		if (State->GetNumSpellsInHand() < MaxSpellCardsInHand)
		{	
			TArray<TSubclassOf<USpellCard>> Pickup = State->PickupCardsFromDeck(1);
			if (Pickup.IsValidIndex(0))
			{
				SpellCard = Pickup[0];
			}
		}

		// The actual amount of gold and mana we gave to this player
		int32 GoldGiven = NewGold - OriginalGold;
		int32 ManaGiven = NewMana - OriginalMana;

		FCollectionPhaseResourcesTally TalliedResults(GoldGiven, ManaGiven, bDeckReshuffled, SpellCard);
		Controller->Client_OnCollectionPhaseResourcesTallied(TalliedResults);
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("ACSKGameMode::UpdatePlayerResources: Unable to update resources for player %i as controller was null!"), PlayerID);
	}
}

void ACSKGameMode::NotifyCollectionPhaseSequenceFinished(ACSKPlayerController* Player)
{
	if (IsCollectionPhaseInProgress())
	{
		CollectionSequenceFinishedFlags |= (1 << Player->CSKPlayerID);
		if (CollectionSequenceFinishedFlags == 0b11)
		{
			FTimerManager& TimerManager = GetWorldTimerManager();
			TimerManager.ClearTimer(Handle_CollectionSequences);

			// Small delay between rounds
			EnterRoundStateAfterDelay(ECSKRoundState::FirstActionPhase, 2.f);
		}
	}
}



bool ACSKGameMode::RequestEndActionPhase(bool bTimeOut)
{
	if (bWinnerSequenceActorSpawned)
	{
		return false;
	}

	// Player is not the active player
	if (!ActionPhaseActiveController || !ActionPhaseActiveController->IsPerformingActionPhase())
	{
		return false;
	}

	// An action request might already be active
	if (!IsActionPhaseInProgress() || IsWaitingForAction())
	{
		return false;
	}

	// Player might not have fulfilled action phase requirements (e.g. moving castle)
	if (!bTimeOut && !ActionPhaseActiveController->CanEndActionPhase())
	{
		return false;
	}

	ActionPhaseActiveController->SetActionPhaseEnabled(false);

	// Move onto next phase
	EnterRoundState(RoundState == ECSKRoundState::FirstActionPhase ? ECSKRoundState::SecondActionPhase : ECSKRoundState::EndRoundPhase);
	return true;
}

bool ACSKGameMode::RequestCastleMove(ATile* Goal)
{
	if (!Goal)
	{
		return false;
	}

	// Player is not the active player
	if (!ActionPhaseActiveController || !ActionPhaseActiveController->IsPerformingActionPhase())
	{
		return false;
	}

	// An action request might already be active
	if (!IsActionPhaseInProgress() || ShouldAcceptRequests())
	{
		return false;
	}

	if (ActionPhaseActiveController->CanRequestCastleMoveAction())
	{
		ACastle* Castle = ActionPhaseActiveController->GetCastlePawn();
		ATile* Origin = Castle->GetCachedTile();

		if (!Origin || Origin == Goal)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastleMove: Move request "
				"denied as castle tile is either invalid or is the move goal"));

			return false;
		}

		// The max amount of tiles this player is allowed to move, a path exceeding this amount will result in being denied
		int32 TileSegments = MaxTileMovements;

		ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
		if (ensure(PlayerState))
		{
			ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
			if (CSKGameState)
			{
				// This player has already traversed the max amount of tiles allowed
				TileSegments = CSKGameState->GetPlayersNumRemainingMoves(PlayerState);
				if (TileSegments == 0)
				{
					return false;
				}
			}
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastleMove: Player may exceed "
				"max tile movements per turn as player state is not of CSKPlayerState"));
		}

		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		check(BoardManager);

		// Confirm request if path is successfully found
		FBoardPath OutBoardPath;
		if (BoardManager->FindPath(Origin, Goal, OutBoardPath, false, TileSegments))
		{
			return ConfirmCastleMove(OutBoardPath);
		}
	}

	return false;
}

bool ACSKGameMode::RequestBuildTower(TSubclassOf<UTowerConstructionData> TowerTemplate, ATile* Tile)
{
	if (!Tile)
	{
		return false;
	}

	// We don't build base towers
	if (!TowerTemplate.Get() || TowerTemplate->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestBuildTower: Tower Template "
			"is invalid. Tower template was either null or was marked as abstract"));
	}

	// Player is not the active player
	if (!ActionPhaseActiveController || !ActionPhaseActiveController->IsPerformingActionPhase())
	{
		return false;
	}

	// An action request might already be active
	if (!IsActionPhaseInProgress() || ShouldAcceptRequests())
	{
		return false;
	}

	if (ActionPhaseActiveController->CanRequestBuildTowerAction())
	{
		// We have to be allowed to place towers on the desired tile
		if (!Tile->CanPlaceTowersOn())
		{
			return false;
		}

		ACastle* Castle = ActionPhaseActiveController->GetCastlePawn();
		ATile* Origin = Castle ? Castle->GetCachedTile() : nullptr;

		if (Origin)
		{
			// The requested tile is not within build range
			if (FHexGrid::HexDisplacement(Origin->GetGridHexValue(), Tile->GetGridHexValue()) > MaxBuildRange)
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestBuildTower: Confirming build request "
				"without range check as players castle cached tile is invalid"));
		}

		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
		if (CSKGameState)
		{
			// The player might not be able to build this tower
			if (!CSKGameState->CanPlayerBuildTower(ActionPhaseActiveController, TowerTemplate))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestBuildTower: Confirming build request "
				"without board validation as game state is not of CSKGameState"));
		}

		UTowerConstructionData* ConstructData = TowerTemplate.GetDefaultObject();
		TSubclassOf<ATower> TowerClass = ConstructData->TowerClass;

		// Confirm request if tower was successfully spawned
		ATower* NewTower = SpawnTowerFor(TowerClass, Tile, ConstructData, ActionPhaseActiveController->GetCSKPlayerState());
		if (NewTower)
		{
			return ConfirmBuildTower(NewTower, Tile, ConstructData);
		}
	}

	return false;
}

bool ACSKGameMode::RequestCastSpell(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* TargetTile, int32 AdditionalMana)
{
	if (!SpellCard.Get() || !TargetTile)
	{
		return false;
	}

	// Player is not the active player
	if (!ActionPhaseActiveController || !ActionPhaseActiveController->IsPerformingActionPhase())
	{
		return false;
	}

	// An action request might already be active
	if (!IsActionPhaseInProgress() || ShouldAcceptRequests())
	{
		return false;
	}

	if (ActionPhaseActiveController->CanRequestCastSpellAction())
	{
		USpellCard* DefaultSpellCard = SpellCard.GetDefaultObject();
		check(DefaultSpellCard);

		// We were given invalid spell
		TSubclassOf<USpell> SpellToActivate = DefaultSpellCard->GetSpellAtIndex(SpellIndex);
		if (!SpellToActivate.Get())
		{
			return false;
		}

		USpell* DefaultSpell = SpellToActivate.GetDefaultObject();
		check(DefaultSpell);

		// Spell is not of correct type
		if (DefaultSpell->GetSpellType() != ESpellType::ActionPhase)
		{
			return false;
		}

		// Spell can't (or there is not point) be cast at tile
		ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
		if (!DefaultSpell->CanActivateSpell(PlayerState, TargetTile))
		{
			return false;
		}

		// The final cost for casting this spell, we pass this to the spell actor
		int32 FinalCost = 0;

		if (PlayerState)
		{
			// Player isn't able to cast another spell (we don't need to check costs)
			if (!PlayerState->CanCastAnotherSpell(false))
			{
				return false;
			}

			// Spell isn't affordable (with discounts applied)
			int32 DiscountedCost = 0;
			if (!PlayerState->GetDiscountedManaIfAffordable(DefaultSpell->GetSpellStaticCost(), DiscountedCost))
			{
				return false;
			}

			// Re-calculate as spell might use additional mana
			FinalCost = DefaultSpell->CalculateFinalCost(PlayerState, TargetTile, DiscountedCost, AdditionalMana);
			if (!PlayerState->HasRequiredMana(FinalCost, true))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastSpell: Confirming spell request "
				"without cost validation as player state is not of CSKPlayerState"));
		}

		// We have confirmed that the player can use their spell, but the opposing
		// player might be able to counter it with a quick effect spell.
		ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		check(OpposingPlayer);

		ACSKPlayerState* OpposingPlayerState = OpposingPlayer->GetCSKPlayerState();
		if (OpposingPlayerState && OpposingPlayerState->CanCastQuickEffectSpell(true))
		{
			SaveActionSpellRequestAndWaitForCounterSelection(SpellCard, SpellIndex, TargetTile, FinalCost, AdditionalMana);
			return true;
		}

		// Confirm request of spell if successfully spawned
		ASpellActor* SpellActor = SpawnSpellActor(DefaultSpell, TargetTile, FinalCost, AdditionalMana, PlayerState);
		if (SpellActor)
		{
			return ConfirmCastSpell(DefaultSpell, DefaultSpellCard, SpellActor, FinalCost, TargetTile, EActiveSpellContext::Action);
		}
	}

	return false;
}

bool ACSKGameMode::RequestCastQuickEffect(TSubclassOf<USpellCard> SpellCard, int32 SpellIndex, ATile* TargetTile, int32 AdditionalMana)
{
	if (!SpellCard.Get() || !TargetTile)
	{
		return false;
	}

	// We might be performing another action (we check move and build
	// individually as quick effect can be activated even if spell action is active)
	if (!IsActionPhaseInProgress() ||
		IsWaitingForCastleMove() || IsWaitingForBuildTower())
	{
		return false;
	}

	// We only accept if we are waiting on a post quick effect selection
	if (IsWaitingForSpellCast())
	{
		if (!bWaitingOnPostQuickEffectSelection || bWaitingOnBonusSpellSelection)
		{
			return false;
		}
	}
	// We might be waiting on a nullify selection
	else if (!bWaitingOnNullifyQuickEffectSelection)
	{
		return false;
	}

	if (bWaitingOnNullifyQuickEffectSelection || bWaitingOnPostQuickEffectSelection)
	{
		USpellCard* DefaultSpellCard = SpellCard.GetDefaultObject();
		check(DefaultSpellCard);

		// We were given invalid spell
		TSubclassOf<USpell> SpellToActivate = DefaultSpellCard->GetSpellAtIndex(SpellIndex);
		if (!SpellToActivate.Get())
		{
			return false;
		}

		USpell* DefaultSpell = SpellToActivate.GetDefaultObject();
		check(DefaultSpell);

		// Spell is not of correct type
		if (DefaultSpell->GetSpellType() != ESpellType::QuickEffect)
		{
			return false;
		}

		// Spell must match what this request if for
		if (bWaitingOnNullifyQuickEffectSelection)
		{
			if (!DefaultSpell->NullifiesOtherSpell())
			{
				return false;
			}
		}
		else
		{
			if (DefaultSpell->NullifiesOtherSpell())
			{
				return false;
			}
		}

		// We check using the opposing player, as only the opposing player can cast quick effects during the active players action phase
		ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		check(OpposingPlayer);

		// Spell can't (or there is no point) be cast at tile
		ACSKPlayerState* PlayerState = OpposingPlayer->GetCSKPlayerState();
		if (!DefaultSpell->CanActivateSpell(PlayerState, TargetTile))
		{
			return false;
		}

		// The final cost for casting this spell, we pass this to the spell actor
		int32 FinalCost = 0;

		if (PlayerState)
		{
			// Spell isn't affordable (with discounts applied)
			int32 DiscountedCost = 0;
			if (!PlayerState->GetDiscountedManaIfAffordable(DefaultSpell->GetSpellStaticCost(), DiscountedCost))
			{
				return false;
			}

			// Re-calculate as spell might use additional mana
			FinalCost = DefaultSpell->CalculateFinalCost(PlayerState, TargetTile, DiscountedCost, AdditionalMana);
			if (!PlayerState->HasRequiredMana(FinalCost, true))
			{
				return false;
			}
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastQuickEffect: Confirming spell request "
				"without cost validation as player state is not of CSKPlayerState"));
		}

		//// The player can cast this spell, but this spell might only
		//// be activatable after the actives player original spell cast
		//if (!DefaultSpell->NullifiesOtherSpell())
		//{
		//	QuickEffectPendingSpellRequest.Set(SpellCard, SpellIndex, TargetTile, FinalCost, AdditionalMana);
		//	RequestSkipQuickEffect();

		//	return true;
		//}
		//else
		//{
		//	QuickEffectPendingSpellRequest.Reset();
		//}

		// Confirm request of spell if successfully spawned
		ASpellActor* SpellActor = SpawnSpellActor(DefaultSpell, TargetTile, FinalCost, AdditionalMana, PlayerState);
		if (SpellActor)
		{
			// We only consume mana from active player if we are casting a nullify quick effect
			return ConfirmCastSpell(DefaultSpell, DefaultSpellCard, SpellActor, FinalCost, 
				TargetTile, EActiveSpellContext::Counter, bWaitingOnPostQuickEffectSelection);
		}
	}

	return false;
}

bool ACSKGameMode::RequestSkipQuickEffect()
{
	if (IsActionPhaseInProgress() && (bWaitingOnNullifyQuickEffectSelection || bWaitingOnPostQuickEffectSelection))
	{
		if (bWaitingOnNullifyQuickEffectSelection)
		{
			ensure(ActivePlayerPendingSpellRequest.IsValid());

			USpellCard* DefaultSpellCard = ActivePlayerPendingSpellRequest.SpellCard.GetDefaultObject();
			check(DefaultSpellCard);

			USpell* DefaultSpell = DefaultSpellCard->GetSpellAtIndex(ActivePlayerPendingSpellRequest.SpellIndex).GetDefaultObject();
			check(DefaultSpell);

			// Resume the initial request
			ASpellActor* SpellActor = SpawnSpellActor(DefaultSpell, ActivePlayerPendingSpellRequest.TargetTile,
				ActivePlayerPendingSpellRequest.CalculatedCost, ActivePlayerPendingSpellRequest.AdditionalMana, ActionPhaseActiveController->GetCSKPlayerState());
			if (SpellActor)
			{
				return ConfirmCastSpell(DefaultSpell, DefaultSpellCard, SpellActor,
					ActivePlayerPendingSpellRequest.CalculatedCost, ActivePlayerPendingSpellRequest.TargetTile, EActiveSpellContext::Action);
			}
		}
		else
		{
			// Finish casting the original spell, we still allow
			// bonus spell to be activated by the original caster
			FinishCastSpell(true, false);
		}
	}

	return false;
}

bool ACSKGameMode::RequestCastBonusSpell(ATile* TargetTile)
{
	if (!TargetTile)
	{
		return false;
	}

	// A spell action should already be active
	if (!IsActionPhaseInProgress() || !IsWaitingForSpellCast())
	{
		return false;
	}

	if (bWaitingOnBonusSpellSelection)
	{
		USpell* DefaultSpell = PendingBonusSpell.GetDefaultObject();
		check(DefaultSpell);

		// Get the player that will be casting this bonus spell
		ACSKPlayerController* CastingPlayer = nullptr;
		if (ActiveSpellContext == EActiveSpellContext::Action)
		{
			CastingPlayer = ActionPhaseActiveController;
		}
		else
		{
			check(ActiveSpellContext == EActiveSpellContext::Counter);
			CastingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		}

		ACSKPlayerState* PlayerState = CastingPlayer->GetCSKPlayerState();
		if (!DefaultSpell->CanActivateSpell(PlayerState, TargetTile))
		{
			return false;
		}

		// Confirm request of spell if successfully spawned
		ASpellActor* SpellActor = SpawnSpellActor(DefaultSpell, TargetTile, 0, 0, PlayerState);
		if (SpellActor)
		{
			BonusSpellContext = ActiveSpellContext;
			return ConfirmCastSpell(DefaultSpell, nullptr, SpellActor, 0, TargetTile, EActiveSpellContext::Bonus);
		}
	}

	return false;
}

bool ACSKGameMode::RequestSkipBonusSpell()
{
	if (IsActionPhaseInProgress() && bWaitingOnBonusSpellSelection)
	{
		FinishCastSpell(true, true);
		bWaitingOnBonusSpellSelection = false;

		return true;
	}

	return false;
}

ASpellActor* ACSKGameMode::CastSubSpellForActiveSpell(TSubclassOf<USpell> SubSpell, ATile* TargetTile, int32 AdditionalMana, int32 OverrideCost)
{
	if (!SubSpell || !TargetTile)
	{
		return nullptr;
	}

	// Sub spells can only be used during spell casts
	if (!(IsActionPhaseInProgress() && IsWaitingForSpellCast()))
	{
		return nullptr;
	}

	check(!bWaitingOnNullifyQuickEffectSelection);

	// While waiting for these types of spells, we are still considered in a spell
	// action, but these spells might be skipped, so we can't allow sub spells to be used
	if (bWaitingOnPostQuickEffectSelection || bWaitingOnBonusSpellSelection)
	{
		return nullptr;
	}

	USpell* DefaultSpell = SubSpell.GetDefaultObject();

	// The player casting the current active spell
	ACSKPlayerController* SpellCaster = nullptr;

	// TODO: We could get the spell caster via (ActiveSpellActor->CastingPlayer->GetCSKPlayerController())
	{
		// We get player based on context but bonus context doesn't specify enough
		EActiveSpellContext CurrentContext = ActiveSpellContext == EActiveSpellContext::Bonus ? BonusSpellContext : ActiveSpellContext;
		switch (CurrentContext)
		{
			case EActiveSpellContext::Action:
			{
				SpellCaster = ActionPhaseActiveController;
				break;
			}
			case EActiveSpellContext::Counter:
			{
				SpellCaster = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
				break;
			}
			default:
			{
				check(false);
				break;
			}
		}
	}

	check(SpellCaster);

	ACSKPlayerState* PlayerState = SpellCaster->GetCSKPlayerState();
	check(PlayerState);

	// Player still needs to be able to afford this spell
	int32 FinalCost = 0;
	{
		// Spell isn't affordable (with discounts applied)
		int32 DiscountedCost = 0;
		if (!PlayerState->GetDiscountedManaIfAffordable(OverrideCost >= 0 ? OverrideCost : DefaultSpell->GetSpellStaticCost(), DiscountedCost))
		{
			return nullptr;
		}

		// Re-calculate as spell might use additional mana
		FinalCost = DefaultSpell->CalculateFinalCost(PlayerState, TargetTile, DiscountedCost, AdditionalMana);
		if (!PlayerState->HasRequiredMana(FinalCost, true))
		{
			return nullptr;
		}
	}

	ASpellActor* SpellActor = SpawnSpellActor(DefaultSpell, TargetTile, FinalCost, AdditionalMana, PlayerState);
	if (SpellActor)
	{
		// Cutdown version of Confirm Cast Spell
		ActiveSubSpellActors.Add(SpellActor);

		PlayerState->SetMana(PlayerState->GetMana() - FinalCost);

		FTimerDelegate DelayedCallback;
		DelayedCallback.BindUObject(this, &ACSKGameMode::OnStartSubSpellCast, SpellActor);

		// Give the sub spell some time to replicate
		FTimerHandle TempHandle;
		FTimerManager& TimerManager = GetWorldTimerManager();
		TimerManager.SetTimer(TempHandle, DelayedCallback, .5f, false);
	}

	return SpellActor;
}

void ACSKGameMode::NotifyCastSpellFinished(ASpellActor* FinishedSpell, bool bWasCancelled)
{
	if (IsActionPhaseInProgress() && IsWaitingForSpellCast())
	{
		if (FinishedSpell->IsPrimarySpell())
		{
			check(FinishedSpell == ActiveSpellActor);
			FinishCastSpell();
		}
		else
		{
			if (ensure(ActiveSubSpellActors.Remove(FinishedSpell) > 0))
			{
				OnSubSpellFinished.ExecuteIfBound(FinishedSpell);
			}

			FinishedSpell->Destroy();
		}
	}
}

void ACSKGameMode::DisableActionModeForActivePlayer(ECSKActionPhaseMode ActionMode)
{
	if (IsActionPhaseInProgress())
	{
		check(ActionPhaseActiveController);

		// Reset the bonus tiles a player can move at the end of a round
		if (ActionMode == ECSKActionPhaseMode::MoveCastle)
		{
			ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
			if (PlayerState)
			{
				PlayerState->SetBonusTileMovements(0);
			}
		}

		// Gets if player has no remaining actions
		if (ActionPhaseActiveController->DisableActionMode(ActionMode))
		{
			if (RoundState == ECSKRoundState::FirstActionPhase)
			{
				EnterRoundState(ECSKRoundState::SecondActionPhase);
			}
			else
			{
				EnterRoundState(ECSKRoundState::EndRoundPhase);
			}
		}
	}
}

bool ACSKGameMode::ConfirmCastleMove(const FBoardPath& BoardPath)
{
	check(IsActionPhaseInProgress());
	check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());

	ACastle* Castle = ActionPhaseActiveController->GetCastlePawn();
	check(Castle);

	// Move castle off of tile
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		check(BoardManager);

		BoardManager->ClearBoardPieceOnTile(Castle->GetCachedTile());
	}

	// Inform castle AI to follow path
	{
		ACastleAIController* CastleController = ActionPhaseActiveController->GetCastleController();
		check(CastleController);

		if (CastleController->FollowPath(BoardPath))
		{
			// Hook callbacks
			UBoardPathFollowingComponent* FollowComp = CastleController->GetBoardPathFollowingComponent();
			Handle_ActivePlayerPathSegment = FollowComp->OnBoardSegmentCompleted.AddUObject(this, &ACSKGameMode::OnActivePlayersPathSegmentComplete);
			Handle_ActivePlayerPathComplete = FollowComp->OnBoardPathFinished.AddUObject(this, &ACSKGameMode::OnActivePlayersPathFollowComplete);

			bWaitingOnActivePlayerMoveAction = true;		
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::ConfirmCastleMove: FollowPath for castle controller failed!"));
			return false;
		}
	}

	// Inform game state
	{
		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
		if (CSKGameState)
		{
			CSKGameState->HandleMoveRequestConfirmed();
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_OnCastleMoveRequestConfirmed(Castle);
			}
		}
	}

	return true;
}

void ACSKGameMode::FinishCastleMove(ATile* DestinationTile)
{
	check(bWaitingOnActivePlayerMoveAction);
	check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());

	// Set castle on tile
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		check(BoardManager);

		BoardManager->PlaceBoardPieceOnTile(ActionPhaseActiveController->GetCastlePawn(), DestinationTile);
	}

	// Unhook callbacks
	{
		ACastleAIController* CastleController = ActionPhaseActiveController->GetCastleController();
		check(CastleController);

		UBoardPathFollowingComponent* FollowComp = CastleController->GetBoardPathFollowingComponent();
		FollowComp->OnBoardSegmentCompleted.Remove(Handle_ActivePlayerPathSegment);
		FollowComp->OnBoardPathFinished.Remove(Handle_ActivePlayerPathComplete);

		bWaitingOnActivePlayerMoveAction = false;		
		Handle_ActivePlayerPathSegment.Reset();
		Handle_ActivePlayerPathComplete.Reset();
	}

	// Inform game state
	{
		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
		if (CSKGameState)
		{
			CSKGameState->HandleMoveRequestFinished();
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_OnCastleMoveRequestFinished();
			}
		}
	}

	// Disable move action if required
	if (bLimitOneMoveActionPerTurn)
	{
		// Disable this player from moving their castle again
		DisableActionModeForActivePlayer(ECSKActionPhaseMode::MoveCastle);
	}
	else
	{
		// This player might be able to move again, check to see if they have moved the max amount of tiles
		ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
		if (PlayerState && PlayerState->GetTilesTraversedThisRound() >= (MaxTileMovements + PlayerState->GetBonusTileMovements()))
		{
			DisableActionModeForActivePlayer(ECSKActionPhaseMode::MoveCastle);
		}
	}
}

void ACSKGameMode::OnActivePlayersPathSegmentComplete(ATile* SegmentTile)
{
	// Keep track of how many tiles this player has moved this round
	ACSKPlayerState* State = ActionPhaseActiveController ? ActionPhaseActiveController->GetCSKPlayerState() : nullptr;
	if (State)
	{
		State->IncrementTilesTraversed();
	}

	// Check if active player has won
	if (PostCastleMoveCheckWinCondition(SegmentTile))
	{
		ACastleAIController* CastleController = ActionPhaseActiveController->GetCastleController();
		check(CastleController);

		CastleController->StopFollowingPath();
	}
}

void ACSKGameMode::OnActivePlayersPathFollowComplete(ATile* DestinationTile)
{
	// Keep track of how many tiles this player has moved this round
	ACSKPlayerState* State = ActionPhaseActiveController ? ActionPhaseActiveController->GetCSKPlayerState() : nullptr;
	if (State)
	{
		State->IncrementTilesTraversed();
	}

	// Check if activer player has won
	if (!PostCastleMoveCheckWinCondition(DestinationTile))
	{
		FinishCastleMove(DestinationTile);
	}
}

bool ACSKGameMode::PostCastleMoveCheckWinCondition(ATile* SegmentTile)
{
	// Can't win post move outside of an action phase
	if (!IsActionPhaseInProgress())
	{
		return false;
	}

	check(ActionPhaseActiveController);

	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		ACSKPlayerController* OpposingController = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		check(OpposingController);

		// Using opposing player ID, get their portal tile to compare
		int32 PlayerID = OpposingController->CSKPlayerID;
		if (BoardManager->GetPlayerPortalTile(PlayerID) == SegmentTile)
		{
			// End the match (play potential winner sequence)
			CacheWinnerAndPlayWinnerSequence(ActionPhaseActiveController, ECSKMatchWinCondition::PortalReached);

			// We want to execute this now as we have already finished the action
			PostActionCheckEndMatch();
			
			return true;
		}
	}

	return false;
}

bool ACSKGameMode::ConfirmBuildTower(ATower* Tower, ATile* Tile, UTowerConstructionData* ConstructData)
{
	check(IsActionPhaseInProgress());
	check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());

	// Hook callbacks
	{
		Handle_ActivePlayerBuildSequenceComplete = Tower->OnBuildSequenceComplete.AddUObject(this, &ACSKGameMode::OnPendingTowerBuildSequenceComplete);

		UHealthComponent* HealthComp = Tower->GetHealthComponent();
		HealthComp->OnHealthChanged.AddDynamic(this, &ACSKGameMode::OnBoardPieceHealthChanged);

		bWaitingOnActivePlayerBuildAction = true;
	}

	// Give tower to player
	{
		ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
		if (PlayerState)
		{
			// Add tower to player
			PlayerState->AddTower(Tower);

			// Consume costs
			PlayerState->SetGold(PlayerState->GetGold() - ConstructData->GoldCost);
			PlayerState->SetMana(PlayerState->GetMana() - ConstructData->ManaCost);
		}
	}

	// Inform game state
	{
		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
		if (CSKGameState)
		{
			CSKGameState->HandleBuildRequestConfirmed(Tile);
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				// We will pass the target tile instead of the new tower,
				// as we don't know if tower will replicate in time
				Controller->Client_OnTowerBuildRequestConfirmed(Tile);
			}
		}
	}

	// Execute tower event (only on server)
	{
		Tower->BP_OnBuiltByPlayer(ActionPhaseActiveController);
	}

	ActivePlayerPendingTower = Tower;
	ActivePlayerPendingTowerTile = Tile;

	// Give tower 2 seconds to replicate
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_ActivePlayerStartBuildSequence, this, &ACSKGameMode::OnStartActivePlayersBuildSequence, 2.f, false);

	return true;
}

void ACSKGameMode::FinishBuildTower()
{
	check(bWaitingOnActivePlayerBuildAction);
	check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());

	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);

	// Unhook callbacks
	{
		ActivePlayerPendingTower->OnBuildSequenceComplete.Remove(Handle_ActivePlayerBuildSequenceComplete);

		bWaitingOnActivePlayerBuildAction = false;
		Handle_ActivePlayerStartBuildSequence.Invalidate();
		Handle_ActivePlayerBuildSequenceComplete.Reset();
	}

	// Inform game state
	{
		if (CSKGameState)
		{
			CSKGameState->HandleBuildRequestFinished(ActivePlayerPendingTower);
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_OnTowerBuildRequestFinished();
			}
		}
	}

	ActivePlayerPendingTower = nullptr;
	ActivePlayerPendingTowerTile = nullptr;

	// Disable this action if player can't build or destroy any more towers this round
	if (CSKGameState && !CSKGameState->CanPlayerBuildMoreTowers(ActionPhaseActiveController))
	{
		DisableActionModeForActivePlayer(ECSKActionPhaseMode::BuildTowers);
	}
}


ATower* ACSKGameMode::SpawnTowerFor(TSubclassOf<ATower> Template, ATile* Tile, UTowerConstructionData* ConstructData, ACSKPlayerState* PlayerState) const
{
	check(Tile);

	if (!Template || !PlayerState)
	{
		return nullptr;
	}

	// Remove scale from transform
	FTransform TileTransform = Tile->GetTransform();
	TileTransform.SetScale3D(FVector::OneVector);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	// Set owner to player we are spawning for, so client
	// RPCs will execute on the client who owns them
	SpawnParams.Owner = PlayerState->GetOwner();

	ATower* Tower = GetWorld()->SpawnActor<ATower>(Template, TileTransform, SpawnParams);
	if (Tower)
	{
		Tower->SetBoardPieceOwnerPlayerState(PlayerState);
		Tower->ConstructData = ConstructData;

		// Towers are moved client side when performing the build sequence. This means while we wait,
		// the actor could pop in and be visible before it moves underground. We can set it to hidden
		// then revert it back once starting the build sequence
		Tower->SetActorHiddenInGame(true);

		Tower->FinishSpawning(TileTransform);
	}

	return Tower;
}

void ACSKGameMode::OnStartActivePlayersBuildSequence()
{
	check(IsActionPhaseInProgress());
	check(ActivePlayerPendingTower && ActivePlayerPendingTowerTile);

	// Place tower on board 
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		check(BoardManager);

		if (!BoardManager->PlaceBoardPieceOnTile(ActivePlayerPendingTower, ActivePlayerPendingTowerTile))
		{
			UE_LOG(LogConquest, Error, TEXT("ACSKGameMode::OnStartActivePlayersBuildSequence: Failed to place "
				"new tower (%s) on the board! Destroying tower"), *ActivePlayerPendingTower->GetName());

			ActivePlayerPendingTower->Destroy();
			
			// TODO: Notify active player that build request failed
		}
	}
}

void ACSKGameMode::OnPendingTowerBuildSequenceComplete()
{
	FinishBuildTower();
}

bool ACSKGameMode::ConfirmCastSpell(USpell* Spell, USpellCard* SpellCard, ASpellActor* SpellActor, int32 FinalCost, 
	ATile* Tile, EActiveSpellContext Context, bool bConsumeOnlyQuickEffect /*= false*/)
{
	check(IsActionPhaseInProgress());
	//check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());
	check(Context != EActiveSpellContext::None);

	// Set states
	{
		bWaitingOnSpellAction = true;
		bWaitingOnNullifyQuickEffectSelection = false;
		bWaitingOnPostQuickEffectSelection = false;
		bWaitingOnBonusSpellSelection = false;
	}

	// Consume mana from player
	{
		// Bonus does not consume any mana from any player
		if (Context != EActiveSpellContext::Bonus)
		{
			// Active player will always consume mana (unless executing a quick effect
			// that runs after the actives player spell cast instead of nullifying it)
			if (!bConsumeOnlyQuickEffect)
			{
				ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();
				if (PlayerState)
				{
					int32 ManaToConsume = FinalCost;
					if (Context == EActiveSpellContext::Counter)
					{
						ensure(ActivePlayerPendingSpellRequest.IsValid());
						ManaToConsume = ActivePlayerPendingSpellRequest.CalculatedCost;
					}

					PlayerState->SetMana(PlayerState->GetMana() - ManaToConsume);

					// If an action, we remove the spell being cast from the active player,
					// as only the active player can cast action spells
					if (Context == EActiveSpellContext::Action)
					{
						PlayerState->RemoveCardFromHand(SpellCard->GetClass());
					}
					else
					{
						PlayerState->RemoveCardFromHand(ActivePlayerPendingSpellRequest.SpellCard);
					}
				}
			}

			if (Context == EActiveSpellContext::Action)
			{
				ActivePlayerPendingSpellRequest.Reset();
			}

			if (Context == EActiveSpellContext::Counter)
			{
				// Consume mana from opposing player if countering
				ACSKPlayerController* OpposingController = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
				ACSKPlayerState* OpposingPlayerState = OpposingController ? OpposingController->GetCSKPlayerState() : nullptr;
				if (OpposingPlayerState)
				{
					OpposingController->ResetQuickEffectSelections();
					OpposingPlayerState->SetMana(OpposingPlayerState->GetMana() - FinalCost);

					// Only the opposing player can use quick effect spell cards
					OpposingPlayerState->RemoveCardFromHand(SpellCard->GetClass());
				}
			}
		}
		else
		{
			// We need to disable the ability for the casting player to select bonus spells
			if (BonusSpellContext == EActiveSpellContext::Action)
			{
				ActionPhaseActiveController->SetBonusSpellSelectionEnabled(false);
			}
			else
			{
				check(BonusSpellContext == EActiveSpellContext::Counter);

				ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
				OpposingPlayer->SetBonusSpellSelectionEnabled(false);
			}
		}
	}

	// Inform game state
	{
		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
		if (CSKGameState)
		{
			CSKGameState->HandleSpellRequestConfirmed(Context, Tile);
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				// We will pass the target tile so players can focus on the target point first
				Controller->Client_OnCastSpellRequestConfirmed(Context, Tile);
			}
		}
	}

	ActiveSpell = Spell;
	ActiveSpellCard = SpellCard;
	ActiveSpellActor = SpellActor;
	ActiveSpellTarget = Tile;
	ActiveSpellContext = Context;

	// Give spell half a second to replicate
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_ExecuteSpellCast, this, &ACSKGameMode::OnStartActiveSpellCast, .5f, false);

	return true;
}

void ACSKGameMode::FinishCastSpell(bool bIgnoreQuickEffectCheck, bool bIgnoreBonusCheck)
{
	check(bWaitingOnSpellAction);
	check(IsActionPhaseInProgress());

	// Destroy sub spells first (there shouldn't be any, but we must be certain)
	for (ASpellActor* SubSpell : ActiveSubSpellActors)
	{
		if (SubSpell && !SubSpell->IsPendingKill())
		{
			UE_LOG(LogConquest, Warning, TEXT("Sub Spell Actor %s is still alive (Not pending Kill) after active "
				"spell has finished execution. Make sure to finish sub spells before finishing active spell"), *SubSpell->GetName());

			//SubSpell->CancelExecution();
			SubSpell->Destroy();
		}
	}

	// We no longer need the spell actor
	if (ActiveSpellActor && !ActiveSpellActor->IsPendingKill())
	{
		ActiveSpellActor->Destroy();
		ActiveSpellActor = nullptr;
	}

	// Some spells will utilize the actions of other spells (mainly quick effects)
	// Save now incase bonus spells or post quick effect spells need them
	CacheAndClearHealthReports();

	// We can now destroy the towers that were destroyed during the spell cast. We do
	// this now so all towers remain valid for spells to use while their still active
	ClearDestroyedTowers();

	// The match might be over due to a castle being destroyed.
	// We want to stop here if that is the case
	if (PostActionCheckEndMatch())
	{
		return;
	}

	if (!bIgnoreQuickEffectCheck)
	{
		// This function can potentially handle the rest for us 
		if (PostCastSpellWaitForCounterSelection())
		{
			return;
		}
	}

	// Used by Skip Bonus Spell
	if (!bIgnoreBonusCheck)
	{
		// This function can potentially handle the rest for us
		if (PostCastSpellActivateBonusSpell())
		{
			return;
		}
	}

	ActiveSpell = nullptr;
	ActiveSpellCard = false;
	ActiveSpellActor = nullptr;
	ActiveSpellTarget = nullptr;

	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	ACSKPlayerState* PlayerState = ActionPhaseActiveController->GetCSKPlayerState();

	// The context of this spell, bonus spells are considered what originally granted the bonus
	EActiveSpellContext Context = ActiveSpellContext == EActiveSpellContext::Bonus ? BonusSpellContext : ActiveSpellContext;

	// Restore states
	{
		bWaitingOnSpellAction = false;

		// We could be skipping a post quick effect request
		if (bWaitingOnPostQuickEffectSelection)
		{
			ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
			OpposingPlayer->ResetQuickEffectSelections();
		}

		// We could be skipping a bonus spell request
		if (bWaitingOnBonusSpellSelection)
		{
			ACSKPlayerController* CasterPlayer = nullptr;
			if (ActiveSpellContext == EActiveSpellContext::Action)
			{
				CasterPlayer = ActionPhaseActiveController;
			}
			else
			{
				check(ActiveSpellContext == EActiveSpellContext::Counter);
				CasterPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
			}

			CasterPlayer->SetBonusSpellSelectionEnabled(false);
		}

		// We only need to disable these two, as nullify will
		// always lead to a confirm spell cast which will do it their
		bWaitingOnPostQuickEffectSelection = false;
		bWaitingOnBonusSpellSelection = false;
	}

	// Increment spells used
	{
		if (Context != EActiveSpellContext::Bonus && PlayerState)
		{
			PlayerState->IncrementSpellsCast(Context == EActiveSpellContext::Counter);
		}
	}

	// Inform game state
	{
		if (CSKGameState)
		{
			CSKGameState->HandleSpellRequestFinished(Context);
		}
	}

	// Inform clients
	{
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_OnCastSpellRequestFinished(Context);
			}
		}
	}
	
	ActiveSpellContext = EActiveSpellContext::None;
	BonusSpellContext = EActiveSpellContext::None;

	// Disable this action if player can't cast any more spells this round
	if (PlayerState && !PlayerState->CanCastAnotherSpell(true))
	{
		DisableActionModeForActivePlayer(ECSKActionPhaseMode::CastSpell);
	}
}

ASpellActor* ACSKGameMode::SpawnSpellActor(USpell* Spell, ATile* Tile, int32 FinalCost, int32 AdditionalMana, ACSKPlayerState* PlayerState) const
{
	check(Tile);

	if (!Spell || !Spell->GetSpellActorClass() || !PlayerState)
	{
		return nullptr;
	}

	TSubclassOf<ASpellActor> SpellActorClass = Spell->GetSpellActorClass();
	FTransform TileTransform = Tile->GetTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	// Set owner to player we are spawning for, so client
	// RPCs will execute on the client who owns them
	SpawnParams.Owner = PlayerState->GetOwner();

	ASpellActor* SpellActor = GetWorld()->SpawnActor<ASpellActor>(SpellActorClass, TileTransform, SpawnParams);
	if (SpellActor)
	{
		SpellActor->InitSpellActor(PlayerState, Spell, FinalCost, AdditionalMana, Tile);
		SpellActor->FinishSpawning(TileTransform);
	}

	return SpellActor;
}

void ACSKGameMode::OnStartActiveSpellCast()
{
	check(IsActionPhaseInProgress());
	check(ActiveSpellActor);

	// Execute spells effect
	{
		ActiveSpellActor->BeginExecution(true);
	}
}

void ACSKGameMode::OnStartSubSpellCast(ASpellActor* SpellActor)
{
	// There is a chance that in between the delay, that the active spell has finished
	if (SpellActor && !SpellActor->IsPendingKill())
	{
		check(IsWaitingForSpellCast());
		SpellActor->BeginExecution(false);
	}
}

void ACSKGameMode::SaveActionSpellRequestAndWaitForCounterSelection(TSubclassOf<USpellCard> InSpellCard, int32 InSpellIndex, ATile* InTargetTile, int32 InFinalCost, int32 InAdditionalMana)
{
	check(IsActionPhaseInProgress());
	check(ActionPhaseActiveController);
	check(!bWaitingOnNullifyQuickEffectSelection && !bWaitingOnPostQuickEffectSelection && !bWaitingOnBonusSpellSelection);

	// Save this for when recieving either request or skip
	ActivePlayerPendingSpellRequest.Set(InSpellCard, InSpellIndex, InTargetTile, InFinalCost, InAdditionalMana);
	bWaitingOnNullifyQuickEffectSelection = true;

	const USpellCard* DefaultSpellCard = InSpellCard.GetDefaultObject();
	check(DefaultSpellCard);

	// Notify clients and game state
	OnStartWaitForCounterSelection(true, DefaultSpellCard->GetSpellAtIndex(InSpellIndex), InTargetTile);
}

bool ACSKGameMode::PostCastSpellWaitForCounterSelection()
{
	check(bWaitingOnSpellAction);
	check(IsActionPhaseInProgress());

	// Post quick effect spells can only be cast after an action spell
	if (ActiveSpellContext != EActiveSpellContext::Action)
	{
		return false;
	}

	check(ActiveSpell && ActiveSpellCard);

	// We check if opposing player has quick effect spells that don't nullify
	ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
	check(OpposingPlayer);

	// Start waiting for the player to counter select if they have at least one spell
	ACSKPlayerState* OpposingPlayerState = OpposingPlayer->GetCSKPlayerState();
	if (OpposingPlayerState && OpposingPlayerState->CanCastQuickEffectSpell(false))
	{
		bWaitingOnPostQuickEffectSelection = true;

		OnStartWaitForCounterSelection(false, ActiveSpell->GetClass(), ActiveSpellTarget);
		return true;
	}

	return false;
}

bool ACSKGameMode::PostCastSpellActivateBonusSpell()
{
	check(bWaitingOnSpellAction);
	check(IsActionPhaseInProgress());

	// Bonuses only get applied once
	if (ActiveSpellContext == EActiveSpellContext::Bonus)
	{
		return false;
	}

	check(ActiveSpell && ActiveSpellCard);

	// Different castle based on spell context, we also need the player
	// state in-case bonus spell requires a specific tile to target
	ACastle* CastersCastle = nullptr;
	ACSKPlayerState* CastersState = nullptr;

	if (ActiveSpellContext == EActiveSpellContext::Action)
	{
		CastersCastle = ActionPhaseActiveController->GetCastlePawn();
		CastersState = ActionPhaseActiveController->GetCSKPlayerState();
	}
	else
	{
		check(ActiveSpellContext == EActiveSpellContext::Counter);

		ACSKPlayerController* OpposingController = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		CastersCastle = OpposingController->GetCastlePawn();
		CastersState = OpposingController->GetCSKPlayerState();
	}

	if (CastersCastle)
	{
		ATile* Tile = CastersCastle->GetCachedTile();
		if (Tile && Tile->TileType != ECSKElementType::None)
		{
			// Check if an element matches
			ECSKElementType MatchingElement = (Tile->TileType & ActiveSpellCard->GetElementalTypes());
			if (BonusElementalSpells.Contains(MatchingElement))
			{
				TSubclassOf<USpell> BonusSpell = BonusElementalSpells[MatchingElement];
				if (!BonusSpell)
				{
					#if WITH_EDITOR
					UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECSKElementType"));
					UE_LOG(LogConquest, Warning, TEXT("Bonus Spell for element %s is invalid"), *EnumClass->GetNameByValue((int64)MatchingElement).ToString());
					#endif

					return false;
				}

				USpell* DefaultSpell = BonusSpell.GetDefaultObject();
				check(DefaultSpell);

				// Spell might require a specific target 
				if (DefaultSpell->RequiresTarget())
				{
					// Wait for the player to give us a target tile
					SaveBonusSpellAndWaitForTargetSelection(BonusSpell);
					return true;
				}

				// We can initiate the spell now if no target is required
				ASpellActor* BonusSpellActor = SpawnSpellActor(DefaultSpell, Tile, 0, 0, CastersState);
				if (BonusSpellActor)
				{
					BonusSpellContext = ActiveSpellContext;
					return ConfirmCastSpell(BonusSpell.GetDefaultObject(), nullptr, BonusSpellActor, 0, Tile, EActiveSpellContext::Bonus);
				}
			}
			#if WITH_EDITOR
			else
			{
				
				// The tile may have been set with multiple element types
				// Spell cards are allowed to have multiple types, but tiles should only have one
				switch (Tile->TileType)
				{
					// Single elements, which are valid
					case ECSKElementType::None:
					case ECSKElementType::Fire:
					case ECSKElementType::Water:
					case ECSKElementType::Earth:
					case ECSKElementType::Air:
					{
						break;
					}
					default:
					{
						UE_LOG(LogConquest, Warning, TEXT("Tile %s has invalid element types. A tile should "
							"only have one element associated with it"), *Tile->GetFName().ToString())
					}
				}			
			}
			#endif
		}
	}

	return false;
}

void ACSKGameMode::SaveBonusSpellAndWaitForTargetSelection(TSubclassOf<USpell> InBonusSpell)
{
	check(IsActionPhaseInProgress());
	check(ActionPhaseActiveController);
	check(!bWaitingOnNullifyQuickEffectSelection && !bWaitingOnBonusSpellSelection);

	PendingBonusSpell = InBonusSpell;
	bWaitingOnBonusSpellSelection = true;

	ACSKPlayerController* CastingPlayer = nullptr;
	ACSKPlayerController* WaitingPlayer = nullptr;
	if (ActiveSpellContext == EActiveSpellContext::Action)
	{
		CastingPlayer = ActionPhaseActiveController;
		WaitingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
	}
	else
	{
		check(ActiveSpellContext == EActiveSpellContext::Counter);
		CastingPlayer = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
		WaitingPlayer = ActionPhaseActiveController;
	}

	// Notify the casting player to select a new spell
	CastingPlayer->SetBonusSpellSelectionEnabled(true);
	CastingPlayer->Client_OnSelectBonusSpellTarget(InBonusSpell);

	// Notify the other player to wait
	WaitingPlayer->Client_OnWaitForBonusSpell();

	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->HandleBonusSpellSelectionStart();
	}
}

void ACSKGameMode::OnStartWaitForCounterSelection(bool bNullify, TSubclassOf<USpell> SpellToCounter, ATile* TargetTile) const
{
	check(SpellToCounter);

	ACSKPlayerController* OpposingController = GetOpposingPlayersController(ActionPhaseActiveController->CSKPlayerID);
	check(OpposingController);

	if (bNullify)
	{
		OpposingController->SetNullifyQuickEffectSelectionEnabled(true);
	}
	else
	{
		OpposingController->SetPostQuickEffectSelectionEnabled(true);
	}

	// Notify the opposing player to select a counter spell;
	OpposingController->Client_OnSelectCounterSpell(bNullify, SpellToCounter, TargetTile);

	// Notify the active player to wait
	ActionPhaseActiveController->Client_OnWaitForCounterSpell(bNullify);

	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->HandleQuickEffectSelectionStart(bNullify);
	}
}



void ACSKGameMode::NotifyEndRoundActionFinished(ATower* Tower)
{
	if ((bRunningTowerEndRoundAction || bInitiatingTowerEndRoundAction) && Tower)
	{
		ClearDestroyedTowers();

		// The match might be over due to a castle being destroyed.
		// We want to stop here if that is the case
		if (PostActionCheckEndMatch())
		{
			return;
		}

		// If we should end the end round phase and move back onto the collection phase
		bool bEndPhase = false;

		if (ensure(EndRoundActionTowers.IsValidIndex(EndRoundRunningTower)))
		{
			// Double check
			//check(EndRoundActionTowers[EndRoundRunningTower] == Tower);
			bRunningTowerEndRoundAction = false;
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode: Index of tower executing end round action is invalid. Forcing end of end round phase"));
			bEndPhase = true;
		}

		// End phase if no more towers are awaiting action execution
		if (!bEndPhase)
		{
			++EndRoundRunningTower;
			bEndPhase = EndRoundRunningTower == EndRoundActionTowers.Num();
		}

		if (bEndPhase)
		{
			// TODO: Notify clients and game state?

			// Start the next round after 2 second delay
			EnterRoundStateAfterDelay(ECSKRoundState::CollectionPhase, 2.f);
		}
		else
		{
			// TODO: Notify clients and game state?

			// Execute the next towers action after 1 second delay
			StartNextEndRoundActionAfterDelay(1.f);
		}
	}
}

// TODO: Optomize by associating towers with a CSKPlayerID before sorting
// right now, the sorting predicate will constantly use get player state which involes casting
bool ACSKGameMode::PrepareEndRoundActionTowers()
{
	// We will most likely have the same amount of tiles as last round
	// (Maybe one or two towers were added since the last round)
	TArray<ATower*> ActionTowers;
	ActionTowers.Reserve(EndRoundActionTowers.Num());

	// First get all towers that will run an action this round
	for (ACSKPlayerController* Controller : Players)
	{
		if (Controller)
		{
			ACSKPlayerState* PlayerState = Controller->GetCSKPlayerState();
			check(PlayerState);

			const TArray<ATower*>& PlayerTower = PlayerState->GetOwnedTowers();
			for (ATower* Tower : PlayerTower)
			{
				// TODO: We might not need to check WantsEndRoundPhaseEvent here anymore!
				if (ensure(Tower) && Tower->WantsEndRoundPhaseEvent())
				{
					ActionTowers.Add(Tower);
				}
			}
		}
	}

	// If two towers happen to share the same priority, we decide
	// who goes first based on the player who won the coin toss 
	int32 PlayerWithPriority = StartingPlayerID;

	// Now sort action towers based on their priority
	ActionTowers.Sort([PlayerWithPriority](const ATower& lhs, const ATower& rhs)->bool
	{
		int32 T1Priority = lhs.GetEndRoundActionPriority();
		int32 T2Priority = rhs.GetEndRoundActionPriority();

		if (T1Priority < T2Priority)
		{
			return true;
		}
		else if (T1Priority > T2Priority)
		{
			return false;
		}
		else
		{
			ACSKPlayerState* T1PlayerState = lhs.GetBoardPieceOwnerPlayerState();
			if (T1PlayerState->GetCSKPlayerID() == PlayerWithPriority)
			{
				// Starting player always takes priority
				return true;
			}
			else
			{
				ACSKPlayerState* T2PlayerState = lhs.GetBoardPieceOwnerPlayerState();
				return T2PlayerState->GetCSKPlayerID() != PlayerWithPriority;
			}
		}
	});

	// We can perform end round phase if there are any towers that can perform an action
	EndRoundActionTowers = MoveTemp(ActionTowers);
	return EndRoundActionTowers.Num() > 0;
}

bool ACSKGameMode::StartRunningTowersEndRoundAction(int32 Index)
{
	bool bResult = false;

	// Safety lock
	bInitiatingTowerEndRoundAction = true;

	if (!bRunningTowerEndRoundAction && EndRoundActionTowers.IsValidIndex(Index))
	{
		check(IsEndRoundPhaseInProgress());

		// This tower might not need to perform it's end round anymore,
		// this could be due to another action affecting it's calculations
		ATower* TowerToRun = EndRoundActionTowers[Index];
		if (!TowerToRun->WantsEndRoundPhaseEvent())
		{
			TowerToRun = nullptr;

			// Keep cycling through each tower till we reach the next tower that can
			while (EndRoundActionTowers.IsValidIndex(++Index))
			{
				ATower* Tower = EndRoundActionTowers[Index];
				if (Tower->WantsEndRoundPhaseEvent())
				{
					TowerToRun = Tower;
					break;
				}
			}
		}

		// Possibility of TowerToRun being null (meaning it or remaining towers no longer need it)
		if (TowerToRun)
		{
			UE_LOG(LogConquest, Log, TEXT("Executing end round action for Tower %s. Action index = %i"), 
				*TowerToRun->GetFName().ToString(), Index + 1);

			// We can track any damage or healing applied (some towers may use it)
			CacheAndClearHealthReports();

			TowerToRun->ExecuteEndRoundPhaseAction();
			bRunningTowerEndRoundAction = true;

			// Track this for use when this towers action ends
			EndRoundRunningTower = Index;

			bResult = true;
		}
	}

	// Safety lock
	bInitiatingTowerEndRoundAction = false;

	if (bResult)
	{
		check(EndRoundActionTowers.IsValidIndex(EndRoundRunningTower));

		// Notify players to concentrate on tower
		ATile* TileWithTower = EndRoundActionTowers[EndRoundRunningTower]->GetCachedTile();
		for (ACSKPlayerController* Controller : Players)
		{
			if (Controller)
			{
				Controller->Client_OnTowerActionStart(TileWithTower);
			}
		}
	}

	return bResult;
}

void ACSKGameMode::StartNextEndRoundActionAfterDelay(float Delay)
{
	if (IsEndRoundPhaseInProgress())
	{
		if (bRunningTowerEndRoundAction)
		{
			return;
		}

		FTimerManager& TimerManager = GetWorldTimerManager();
		if (TimerManager.IsTimerActive(Handle_DelayEndRoundAction))
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::StartNextEndRoundActionAfterDelay: Delay is already set, replacing it with new delay"));
			TimerManager.ClearTimer(Handle_DelayEndRoundAction);
		}

		if (Delay > 0.f)
		{
			TimerManager.SetTimer(Handle_DelayEndRoundAction, this, &ACSKGameMode::OnStartNextEndRoundAction, Delay);
		}
		else
		{
			TimerManager.SetTimerForNextTick(this, &ACSKGameMode::OnStartNextEndRoundAction);
			Handle_DelayEndRoundAction.Invalidate();
		}
	}
}

void ACSKGameMode::OnStartNextEndRoundAction()
{
	if (IsEndRoundPhaseInProgress() && !StartRunningTowersEndRoundAction(EndRoundRunningTower))
	{
		EnterRoundStateAfterDelay(ECSKRoundState::CollectionPhase, 1.f);
	}
}

void ACSKGameMode::GiveGoldToPlayer(ACSKPlayerState* PlayerState, int32 Amount) const
{
	if (PlayerState)
	{
		int32 NewGold = PlayerState->GetGold() + Amount;
		PlayerState->SetGold(ClampGoldToLimit(NewGold));
	}
}

void ACSKGameMode::GiveManaToPlayer(ACSKPlayerState* PlayerState, int32 Amount) const
{
	if (PlayerState)
	{
		int32 NewMana = PlayerState->GetMana() + Amount;
		PlayerState->SetMana(ClampManaToLimit(NewMana));
	}
}

void ACSKGameMode::OnBoardPieceHealthChanged(UHealthComponent* HealthComp, int32 NewHealth, int32 Delta)
{
	// Get script interface as damage board piece could either be a castle or tower
	AActor* CompOwner = HealthComp->GetOwner();
	TScriptInterface<IBoardPieceInterface> BoardPieice(CompOwner);

	if (!BoardPieice)
	{
		return;
	}

	ACSKPlayerState* PlayerState = BoardPieice->GetBoardPieceOwnerPlayerState();
	check(PlayerState);

	// This is used to generate the health report
	bool bIsCastle = CompOwner->IsA<ACastle>();
	bool bKilled = HealthComp->IsDead();

	if (bKilled)
	{
		ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);

		if (CompOwner->IsA<ACastle>())
		{
			ACastle* DestroyedCastle = static_cast<ACastle*>(CompOwner);
			ACSKPlayerController* OpposingPlayer = GetOpposingPlayersController(PlayerState->GetCSKPlayerID());

			// Notify game state
			if (CSKGameState)
			{
				CSKGameState->HandleCastleDestroyed(OpposingPlayer, DestroyedCastle);
			}

			// End the match (after a potential win sequence)
			CacheWinnerAndPlayWinnerSequence(OpposingPlayer, ECSKMatchWinCondition::CastleDestroyed);
		}
		else
		{
			ATower* DestroyedTower = CastChecked<ATower>(CompOwner);

			// Remove references from both board and player
			{
				ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
				BoardManager->ClearBoardPieceOnTile(DestroyedTower->GetCachedTile());

				PlayerState->RemoveTower(DestroyedTower);
			}

			// Allow tower to clean up itself
			{
				DestroyedTower->SetActorHiddenInGame(true);
				DestroyedTower->BP_OnDestroyed(PlayerState->GetCSKPlayerController());
			}

			// Notify game state
			if (CSKGameState)
			{
				CSKGameState->HandleTowerDestroyed(DestroyedTower, false);
			}

			// We need to make sure this tower is removed from end round phase actions
			if (IsEndRoundPhaseInProgress())
			{
				// This tower could possibly be a tower with an end round action
				int32 Index = EndRoundActionTowers.Find(DestroyedTower);
				if (Index != -1)
				{
					if (Index == EndRoundRunningTower)
					{
						check(EndRoundActionTowers[Index] == DestroyedTower);
						UE_LOG(LogConquest, Warning, TEXT("Tower %s has destroyed itself during it's action. "
							"Is this intended?"), *DestroyedTower->GetFName().ToString());

						// We let this tower finish it's execution
					}

					// We need to revert the index back one if this tower has already,
					// executed as not doing so will skip one towers end round action
					if (Index >= EndRoundRunningTower)
					{
						--EndRoundRunningTower;
					}

					EndRoundActionTowers.RemoveAt(Index);
				}
			}

			// Cache this tower to be destroyed after the current action
			ActiveActionsDestroyedTowers.Add(DestroyedTower);
		}

		UE_LOG(LogConquest, Log, TEXT("Board piece %s (owned by Player %i) has been destroyed"),
			*CompOwner->GetName(), PlayerState->GetCSKPlayerID() + 1);
	}
	else
	{
		// Positive delta = Healing, Negative delta = Damage
		if (Delta > 0)
		{
			UE_LOG(LogConquest, Log, TEXT("Board piece %s (owned by Player %i) has recovered %i health"),
				*CompOwner->GetName(), PlayerState->GetCSKPlayerID() + 1, Delta);
		}
		else
		{
			UE_LOG(LogConquest, Log, TEXT("Board piece %s (owned by Player %i) has recieved %i damage"),
				*CompOwner->GetName(), PlayerState->GetCSKPlayerID() + 1, Delta);
		}
	}

	// Generate a change report and save it
	{
		FHealthChangeReport Report(CompOwner, PlayerState, bIsCastle, bKilled, Delta);
		ActiveActionHealthReports.Add(MoveTemp(Report));
	}
}

void ACSKGameMode::ClearHealthReports()
{
	ActiveActionHealthReports.Empty();
	PreviousActionHealthReports.Empty();

	ACSKGameState* CSKGameState = CastChecked<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->SetLatestActionHealthReports(PreviousActionHealthReports);
	}
}

void ACSKGameMode::CacheAndClearHealthReports()
{
	PreviousActionHealthReports = MoveTemp(ActiveActionHealthReports);

	ACSKGameState* CSKGameState = CastChecked<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->SetLatestActionHealthReports(PreviousActionHealthReports);
	}
}

void ACSKGameMode::ClearDestroyedTowers()
{
	// We move over the towers for two reasons:
	// 1. It empties the active actions array
	// 2. Destroying objects in a UPROPERTY marked array are immediately removed from said array
	// Meaning it's annoying to loop through each actor in order to simply destroy it
	TArray<ATower*> TowersToDestroy = MoveTemp(ActiveActionsDestroyedTowers);
	for (ATower* Tower : TowersToDestroy)
	{
		if (Tower)
		{
			Tower->Destroy();
		}
	}
}

AWinnerSequenceActor* ACSKGameMode::SpawnWinnerSequenceActor(ACSKPlayerState* Winner, ECSKMatchWinCondition WinCondition) const
{
	if (!Winner)
	{
		return nullptr;
	}

	// We spawn a different sequence actor based on the win condition
	TSubclassOf<AWinnerSequenceActor> SequenceClass = nullptr;
	switch (WinCondition)
	{
		case ECSKMatchWinCondition::PortalReached:
		{
			SequenceClass = PortalReachedSequenceClass;
			break;
		}
		case ECSKMatchWinCondition::CastleDestroyed:
		{
			SequenceClass = CastleDestroyedSequenceClass;
			break;
		}
	}
	
	if (SequenceClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bDeferConstruction = true;
		
		AWinnerSequenceActor* SequenceActor = GetWorld()->SpawnActor<AWinnerSequenceActor>(SequenceClass, SpawnParams);
		if (SequenceActor)
		{
			SequenceActor->InitSequenceActor(Winner, WinCondition);
			SequenceActor->FinishSpawning(FTransform::Identity);
		}

		return SequenceActor;
	}

	return nullptr;
}

void ACSKGameMode::OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver)
{
	UNetConnection* NetConnection = NetDriver->ServerConnection;
	if (NetConnection && NetConnection->PlayerController)
	{
		ACSKPlayerController* Controller = Cast<ACSKPlayerController>(NetConnection->PlayerController);
		if (Controller)
		{
			//PlayersLeft |= Controller->CSKPlayerID + 1;
		}
	}
}

#undef LOCTEXT_NAMESPACE