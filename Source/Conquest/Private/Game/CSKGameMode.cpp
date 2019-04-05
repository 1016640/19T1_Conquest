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

	ActionPhaseTime = 90.f;
	BonusActionPhaseTime = 20.f;
	MinTileMovements = 1;
	MaxTileMovements = 2;
	MaxTowerConstructs = 0;
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
			ACastle* Castle = SpawnCastleAtPortal(PlayerID, CastleTemplate);
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

ACastle* ACSKGameMode::SpawnCastleAtPortal(int32 PlayerID, TSubclassOf<ACastle> Class) const
{
	check(Class);

	const ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		ATile* PortalTile = BoardManager->GetPlayerPortalTile(PlayerID);
		if (PortalTile)
		{
			FTransform TileTransform = PortalTile->GetTransform();

			FActorSpawnParameters SpawnParams;
			SpawnParams.ObjectFlags |= RF_Transient;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			//SpawnParams.bDeferConstruction = true;

			ACastle* Castle = GetWorld()->SpawnActor<ACastle>(Class, TileTransform, SpawnParams);
			if (!Castle)
			{
				UE_LOG(LogConquest, Warning, TEXT("Failed to spawn castle for Player %i"), PlayerID + 1);
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

bool ACSKGameMode::IsActionPhaseInProgress() const
{
	if (IsMatchInProgress())
	{
		return RoundState == ECSKRoundState::FirstActionPhase || RoundState == ECSKRoundState::SecondActionPhase;
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
	CollectResourcesForPlayers();

	UKismetSystemLibrary::PrintString(this, "Collection Phase Start", true, false, FLinearColor::Blue, 5.f);

	FTimerHandle Temp;
	GetWorldTimerManager().SetTimer(Temp, this, &ACSKGameMode::GotoActionPhase1, 2.f, false);
}

void ACSKGameMode::OnFirstActionPhaseStart()
{
	ActionPhaseActiveController = GetActivePlayerForActionPhase(0);
	ActionPhaseActiveController->SetActionPhaseEnabled(true);
	
	bWaitingOnActivePlayerMoveAction = false;

	UKismetSystemLibrary::PrintString(this, FString("Player ") + FString::FromInt(ActionPhaseActiveController->CSKPlayerID), true, false, FLinearColor::Blue, 5.f);
	UKismetSystemLibrary::PrintString(this, "First player action phase start", true, false, FLinearColor::Blue, 5.f);
}

void ACSKGameMode::OnSecondActionPhaseStart()
{
	ActionPhaseActiveController->SetActionPhaseEnabled(false);
	ActionPhaseActiveController = GetActivePlayerForActionPhase(1);
	ActionPhaseActiveController->SetActionPhaseEnabled(true);

	bWaitingOnActivePlayerMoveAction = false;

	UKismetSystemLibrary::PrintString(this, FString("Player ") + FString::FromInt(ActionPhaseActiveController->CSKPlayerID), true, false, FLinearColor::Blue, 5.f);
	UKismetSystemLibrary::PrintString(this, "Second player action phase start", true, false, FLinearColor::Blue, 5.f);
}

void ACSKGameMode::OnEndRoundPhaseStart()
{
	ActionPhaseActiveController->SetActionPhaseEnabled(false);
	ActionPhaseActiveController = nullptr;

	UKismetSystemLibrary::PrintString(this, "End round Phase Start", true, false, FLinearColor::Blue, 5.f );

	FTimerHandle Temp;
	GetWorldTimerManager().SetTimer(Temp, this, &ACSKGameMode::GototCollectPhase, 2.f, false);
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

	// TODO: Add check if round state is right and move action is allowed
	// for now
	if (bWaitingOnActivePlayerMoveAction)
	{
		return false;
	}

	// This player might have already used all their moves
	if (ActionPhaseActiveController->CanRequestCastleMoveAction())
	{
		ACastle* Castle = ActionPhaseActiveController->GetCastlePawn();
		ATile* Origin = Castle->GetCachedTile();

		if (!Origin || Origin == Goal)
		{
			UE_LOG(LogConquest, Warning, TEXT("ACSKGameMode::RequestCastleMove: Move request "
				"denied as castle tile is either invalid or the move goal"));

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
	check(IsActionPhaseInProgress());
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

	ActionPhaseActiveController->OnMoveActionFinished();

	// For now
	if (RoundState == ECSKRoundState::FirstActionPhase)
	{
		EnterRoundState(ECSKRoundState::SecondActionPhase);
	}
	else
	{
		EnterRoundState(ECSKRoundState::EndRoundPhase);
	}
}

void ACSKGameMode::OnActivePlayersPathSegmentComplete(ATile* SegmentTile)
{
	// Check if active player has won
	if (!PostCastleMoveCheckWinCondition(SegmentTile))
	{
		// We prob don't need to do anything here (PostCastleMove should handle it)
	}
}

void ACSKGameMode::OnActivePlayersPathFollowComplete(ATile* DestinationTile)
{
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
			UE_LOG(LogConquest, Warning, TEXT("Unable to reset resources for player %i as player state is invalid"), PlayerID);
			return;
		}

		State->GiveResources(StartingGold, StartingMana);
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("Unable to reset resources for player %i as controller was null!"), PlayerID);
	}
}

void ACSKGameMode::UpdatePlayerResources(ACSKPlayerController* Controller, int32 PlayerID)
{
	if (ensure(Controller))
	{
		ACSKPlayerState* State = Controller->GetCSKPlayerState();
		if (!ensure(State))
		{
			UE_LOG(LogConquest, Warning, TEXT("Unable to update resources for player %i as player state is invalid"), PlayerID);
			return;
		}

		int32 GoldToGive = CollectionPhaseGold;
		int32 ManaToGive = CollectionPhaseMana;

		// TODO: Cycle through players owned buildings to collect additional resources

		State->GiveResources(GoldToGive, ManaToGive);
	}
	else
	{
		UE_LOG(LogConquest, Error, TEXT("Unable to update resources for player %i as controller was null!"), PlayerID);
	}
}

void ACSKGameMode::OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver)
{
	AbortMatch();
}

void ACSKGameMode::GotoActionPhase1()
{
	EnterRoundState(ECSKRoundState::FirstActionPhase);
}

void ACSKGameMode::GototCollectPhase()
{
	EnterRoundState(ECSKRoundState::CollectionPhase);
}

#undef LOCTEXT_NAMESPACE