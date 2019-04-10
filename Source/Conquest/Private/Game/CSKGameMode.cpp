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
#include "GameDelegates.h"
#include "Tile.h"
#include "TimerManager.h"
#include "Tower.h"
#include "TowerConstructionData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

// temp
#include "Kismet/KismetSystemLibrary.h"

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

	StartingGold = 5;
	StartingMana = 3;
	CollectionPhaseGold = 3;
	CollectionPhaseMana = 2;
	MaxGold = 30;
	MaxMana = 30;

	MaxNumTowers = 7;
	MaxNumLegendaryTowers = 1;
	MaxNumDuplicatedTowers = 2;
	MaxBuildRange = 4;

	ActionPhaseTime = 90.f;
	BonusActionPhaseTime = 20.f;
	MinTileMovements = 1;
	MaxTileMovements = 2;
	bLimitOneMoveActionPerTurn = false;
}

void ACSKGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep checking till we can start the match
	if (GameState->GetServerWorldTimeSeconds() > 2.f && ShouldStartMatch())
	{
		StartMatch();
	}
}

void ACSKGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Players.Add(nullptr);
	Players.Add(nullptr);

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

void ACSKGameMode::PostLogin(APlayerController* NewPlayer)
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

	Super::PostLogin(NewPlayer);
}

void ACSKGameMode::Logout(AController* Exiting)
{
	if (MatchState != ECSKMatchState::LeavingGame)
	{
		// TODO: Would want to have the exit option inform the server to end the game instead of abort like this
		AbortMatch();
	}

	Super::Logout(Exiting);
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
			FTransform TileTransform = PortalTile->GetTransform();

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
			}
		}
	}
}

void ACSKGameMode::EnterMatchState(ECSKMatchState NewState)
{
	if (NewState != MatchState)
	{
		#if WITH_EDITOR
		UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECSKMatchState"));
		UE_LOG(LogConquest, Log, TEXT("Entering Match State: %s"), *EnumClass->GetNameByIndex((int32)NewState).ToString());
		#endif
	
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

		#if WITH_EDITOR
		UEnum* EnumClass = FindObject<UEnum>(ANY_PACKAGE, TEXT("ECSKRoundState"));
		UE_LOG(LogConquest, Log, TEXT("Entering Round State: %s"), *EnumClass->GetNameByIndex((int32)NewState).ToString());
		#endif

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
		// Start the match
		EnterMatchState(ECSKMatchState::Running);
	}
}

void ACSKGameMode::EndMatch()
{
	if (ShouldEndMatch())
	{
		// End the match
		EnterMatchState(ECSKMatchState::WaitingPostMatch);
	}
}

void ACSKGameMode::AbortMatch()
{
	// Abandon the match
	EnterMatchState(ECSKMatchState::Aborted);
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
	// TODO: Check match state
	return false;
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
}

// TODO: Fixup this process once I get a better understanding
void ACSKGameMode::OnMatchStart()
{
	// Restart all players (even those that aren't participating
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

	// Give players the default resources
	ResetResourcesForPlayers();

	// For now (move to coin flip section)
	StartingPlayerID = CoinFlip() ? 0 : 1;

	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();
	WorldSettings->NotifyMatchStarted();

	// Have each player transition to their board
	for (int32 i = 0; i < CSK_MAX_NUM_PLAYERS; ++i)
	{
		Players[i]->OnTransitionToBoard();
	}

	SetActorTickEnabled(false);

	EnterRoundState(ECSKRoundState::CollectionPhase);
}

void ACSKGameMode::OnMatchFinished()
{
}

void ACSKGameMode::OnFinishedWaitingPostMatch()
{
}

void ACSKGameMode::OnMatchAbort()
{
}

void ACSKGameMode::OnCollectionPhaseStart()
{
	UE_LOG(LogConquest, Log, TEXT("Starting Collection Phase"));

	CollectionSequenceFinishedFlags = 0;
	DelayThenStartCollectionPhaseSequence();

	UKismetSystemLibrary::PrintString(this, "Collection Phase Start", true, false, FLinearColor::Blue, 5.f);
}

void ACSKGameMode::OnFirstActionPhaseStart()
{
	UE_LOG(LogConquest, Log, TEXT("Starting Action Phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);

	Handle_CollectionSequences.Invalidate();

	UpdateActivePlayerForActionPhase(0);
	check(ActionPhaseActiveController);

	UKismetSystemLibrary::PrintString(this, FString("Player ") + FString::FromInt(ActionPhaseActiveController->CSKPlayerID + 1), true, false, FLinearColor::Blue, 5.f);
	UKismetSystemLibrary::PrintString(this, "First player action phase start", true, false, FLinearColor::Blue, 5.f);
}

void ACSKGameMode::OnSecondActionPhaseStart()
{
	UE_LOG(LogConquest, Log, TEXT("Starting Action Phase for Player %i"), ActionPhaseActiveController->CSKPlayerID + 1);

	UpdateActivePlayerForActionPhase(1);
	check(ActionPhaseActiveController);

	UKismetSystemLibrary::PrintString(this, FString("Player ") + FString::FromInt(ActionPhaseActiveController->CSKPlayerID), true, false, FLinearColor::Blue, 5.f);
	UKismetSystemLibrary::PrintString(this, "Second player action phase start", true, false, FLinearColor::Blue, 5.f);

	
}

void ACSKGameMode::OnEndRoundPhaseStart()
{
	UE_LOG(LogConquest, Log, TEXT("Starting End Round Phase"));

	// Resets action phase active controller
	UpdateActivePlayerForActionPhase(-1);

	EndRoundRunningTower = 0;
	bRunningTowerEndRoundAction = false;
	bInitiatingTowerEndRoundAction = false;

	if (PrepareEndRoundActionTowers())
	{
		if (!StartRunningTowersEndRoundAction(0))
		{
			EnterRoundStateAfterDelay(ECSKRoundState::CollectionPhase, 2.f);
		}
	}
	else
	{
		ACSKGameState* CSKGameState = CastChecked<ACSKGameState>(GameState);
		UE_LOG(LogConquest, Log, TEXT("Skipping End Round Phase for round %i as no placed towers can perform actions"), CSKGameState->GetRound());

		EnterRoundStateAfterDelay(ECSKRoundState::CollectionPhase, 2.f);
	}

	UKismetSystemLibrary::PrintString(this, "End round Phase Start", true, false, FLinearColor::Blue, 5.f );
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
			"a new delay has been requested (This might be called during that delays execution though!)."));
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





bool ACSKGameMode::CoinFlip_Implementation() const
{
	int32 RandomValue = FMath::Rand();
	return (RandomValue % 2) == 1;
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

		State->SetResources(StartingGold, StartingMana);
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
		int32 SpellUsesToGive = 1;

		// Cycle through this players towers to collect any additional resources
		// TODO: Designers want camera to pan to each tower as it shows how much resources it's given the player. This
		// will force this function to be split up (not all resources have to be given at once)
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

		int32 NewGold = ClampGoldToLimit(State->GetGold() + GoldToGive);
		int32 NewMana = ClampManaToLimit(State->GetMana() + ManaToGive);
		State->SetResources(NewGold, NewMana);

		FCollectionPhaseResourcesTally TalliedResults(GoldToGive, ManaToGive, SpellUsesToGive);
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



bool ACSKGameMode::RequestEndActionPhase()
{
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
	if (!ActionPhaseActiveController->CanEndActionPhase())
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
	if (!IsActionPhaseInProgress() || IsWaitingForAction())
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
			TileSegments -= PlayerState->GetTilesTraversedThisRound();
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastleMove: Player may exceed "
				"max tile movements per turn as player state is not of CSKPlayerState"));
		}

		if (IsCountWithinTileTravelLimits(TileSegments))
		{
			ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
			check(BoardManager);

			// Confirm request if path is successfully found
			FBoardPath OutBoardPath;
			if (BoardManager->FindPath(Origin, Goal, OutBoardPath, false, TileSegments))
			{
				return ConfirmCastleMove(OutBoardPath);
			}
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
		UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestBuildTowe: Tower Template "
			"is invalid. Tower template was either null or was marked as abstract"));
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
			if (!CSKGameState->CanPlayerBuildTower(ActionPhaseActiveController, TowerTemplate, false))
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
		ATower* NewTower = SpawnTowerFor(TowerClass, Tile, ActionPhaseActiveController->GetCSKPlayerState());
		if (NewTower)
		{
			return ConfirmBuildTower(NewTower, Tile, ConstructData);
		}
	}

	return false;
}

void ACSKGameMode::DisableActionModeForActivePlayer(ECSKActionPhaseMode ActionMode)
{
	if (IsActionPhaseInProgress())
	{
		check(ActionPhaseActiveController);
		if (ActionPhaseActiveController->DisableActionMode(ActionMode))
		{
			// will also prob want to add some small delays here to make sure
			// all changes reach all clients
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
			Controller->Client_OnCastleMoveRequestConfirmed(Castle);
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
			Controller->Client_OnCastleMoveRequestFinished();
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
		if (PlayerState && PlayerState->GetTilesTraversedThisRound() >= MaxTileMovements)
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
	if (!PostCastleMoveCheckWinCondition(SegmentTile))
	{
		// We prob don't need to do anything here (PostCastleMove should handle it)
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
			// TODO: End game with winner (we would also want to stop movement for the castle)
			return false;// true;
		}
	}

	return false;
}

bool ACSKGameMode::ConfirmBuildTower(ATower* Tower, ATile* Tile, UTowerConstructionData* ConstructData)
{
	check(IsActionPhaseInProgress());
	check(ActionPhaseActiveController && ActionPhaseActiveController->IsPerformingActionPhase());

	// TODO: hook callbacks (both in general and potential build anim)
	{
		Handle_ActivePlayerBuildSequenceComplete = Tower->OnBuildSequenceComplete.AddUObject(this, &ACSKGameMode::OnPendingTowerBuildSequenceComplete);

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
			// We will pass the target tile instead of the new tower,
			// as we don't know if tower will replicate in time
			Controller->Client_OnTowerBuildRequestConfirmed(Tile);
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
			Controller->Client_OnTowerBuildRequestFinished();
		}
	}

	ActivePlayerPendingTower = nullptr;
	ActivePlayerPendingTowerTile = nullptr;

	// Disable this action if player can't build or destroy any more towers this round
	if (CSKGameState && !CSKGameState->CanPlayerBuildMoreTowers(ActionPhaseActiveController, true))
	{
		DisableActionModeForActivePlayer(ECSKActionPhaseMode::BuildTowers);
	}
}


ATower* ACSKGameMode::SpawnTowerFor(TSubclassOf<ATower> Template, ATile* Tile, ACSKPlayerState* PlayerState) const
{
	check(Tile);

	if (!Template || !PlayerState)
	{
		return nullptr;
	}

	FTransform TileTransform = Tile->GetTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	ATower* Tower = GetWorld()->SpawnActor<ATower>(Template, TileTransform, SpawnParams);
	if (Tower)
	{
		Tower->SetBoardPieceOwnerPlayerState(PlayerState);

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

void ACSKGameMode::NotifyEndRoundActionFinished(ATower* Tower)
{
	if (bRunningTowerEndRoundAction && Tower)
	{
		// If we should end the end round phase and move back onto the collection phase
		bool bEndPhase = false;

		if (ensure(EndRoundActionTowers.IsValidIndex(EndRoundRunningTower)))
		{
			// Double check
			check(EndRoundActionTowers[EndRoundRunningTower] == Tower);
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
			// TODO: Notify clients and game state

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
		ACSKPlayerState* PlayerState = Controller->GetCSKPlayerState();
		check(PlayerState);

		const TArray<ATower*>& PlayerTower = PlayerState->GetOwnedTowers();
		for (ATower* Tower : PlayerTower)
		{
			if (ensure(Tower) && Tower->WantsEndRoundPhaseEvent())
			{
				ActionTowers.Add(Tower);
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

		if (T1Priority > T2Priority)
		{
			return true;
		}
		else if (T1Priority < T2Priority)
		{
			return false;
		}
		else
		{
			ACSKPlayerState* T1PlayerState = lhs.GetOwnerPlayerState();
			if (T1PlayerState->GetCSKPlayerID() == PlayerWithPriority)
			{
				// Starting player always takes priority
				return true;
			}
			else
			{
				ACSKPlayerState* T2PlayerState = lhs.GetOwnerPlayerState();
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

		ATower* TowerToRun = EndRoundActionTowers[Index];
		if (ensure(TowerToRun))
		{
			TowerToRun->ExecuteEndRoundPhaseAction();
			bRunningTowerEndRoundAction = true;

			// Track this for use when this towers action ends
			EndRoundRunningTower = Index;

			bResult = true;
		}
	}

	// Safety lock
	bInitiatingTowerEndRoundAction = false;

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



void ACSKGameMode::OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver)
{
	AbortMatch();
}

#undef LOCTEXT_NAMESPACE