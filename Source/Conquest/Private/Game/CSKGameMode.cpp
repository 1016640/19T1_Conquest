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
#include "Castle.h"
#include "CastleAIController.h"
#include "GameDelegates.h"
#include "Tile.h"
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

	Player1PC = nullptr;
	Player2PC = nullptr;
	Player1CastleClass = ACastle::StaticClass();
	Player2CastleClass = ACastle::StaticClass();
	CastleAIControllerClass = ACastleAIController::StaticClass();

	StartingGold = 5;
	StartingMana = 3;
	CollectionPhaseGold = 3;
	CollectionPhaseMana = 2;
	MaxGold = 30;
	MaxMana = 30;

	ActionPhaseTime = 90.f;
	MinTileMovements = 1;
	MaxTileMovements = 2;
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

void ACSKGameMode::InitGameState()
{
	Super::InitGameState();

	// First tell game state to save the board we are using for this match
	ACSKGameState* CSKGameState = Cast<ACSKGameState>(GameState);
	if (CSKGameState)
	{
		CSKGameState->SetMatchBoardManager(UConquestFunctionLibrary::FindMatchBoardManager(this));
	}

	EnterMatchState(ECSKMatchState::EnteringGame);

	// Callbacks required
	FGameDelegates& GameDelegates = FGameDelegates::Get();
	GameDelegates.GetHandleDisconnectDelegate().AddUObject(this, &ACSKGameMode::OnDisconnect);
}

void ACSKGameMode::StartPlay()
{
	if (MatchState == ECSKMatchState::EnteringGame)
	{
		EnterMatchState(ECSKMatchState::WaitingPreMatch);
	}

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
		if (Player1PC)
		{		
			// We should only ever have two players
			if (!ensure(!Player2PC))
			{
				UE_LOG(LogConquest, Error, TEXT("More than 2 people of joined a CSK match even though only 2 max are allowed"));
			}
			else
			{
				Player2PC = CastChecked<ACSKPlayerController>(NewPlayer);
			}
		}
		else
		{
			Player1PC = CastChecked<ACSKPlayerController>(NewPlayer);
		}

	}

	Super::PostLogin(NewPlayer);
}

void ACSKGameMode::Logout(AController* Exiting)
{
	if (GetControllerAsPlayerID(Exiting))
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
		return (Player1PC != nullptr && Player2PC != nullptr);
	}

	return true;
}

ACastleAIController* ACSKGameMode::SpawnDefaultCastleFor(ACSKPlayerController* NewPlayer) const
{
	int32 PlayerID = GetControllerAsPlayerID(NewPlayer);
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

int32 ACSKGameMode::GetControllerAsPlayerID(AController* Controller) const
{
	if (Controller)
	{
		if (Controller == Player1PC)
		{
			return 0;
		}

		if (Controller == Player2PC)
		{
			return 1;
		}
	}

	return -1;
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
		HandleStateChange(OldState, NewState);

		// Inform clients of change
		ACSKGameState* CSKGameState = GetGameState<ACSKGameState>();
		if (CSKGameState)
		{
			CSKGameState->SetMatchState(NewState);
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
	if (Player1PC && Player2PC)
	{
		return Player1PC->HasClientLoadedCurrentWorld() && Player2PC->HasClientLoadedCurrentWorld();
	}

	return false;
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

void ACSKGameMode::OnStartWaitingPreMatch()
{
	// Allow actors in the world to start ticking while we wait
	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();
}

// TODO: Fixup this process once I get a better understanding
void ACSKGameMode::OnMatchStart()
{
	// Restart all players
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

	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();
	WorldSettings->NotifyMatchStarted();

	// Have each player transition to their board
	Player1PC->OnTransitionToBoard(0);
	Player2PC->OnTransitionToBoard(1);

	SetActorTickEnabled(false);
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

void ACSKGameMode::HandleStateChange(ECSKMatchState OldState, ECSKMatchState NewState)
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

void ACSKGameMode::OnDisconnect(UWorld* InWorld, UNetDriver* NetDriver)
{
	AbortMatch();
}

bool ACSKGameMode::CoinFlip() const
{
	return UConquestFunctionLibrary::CoinFlip();
}

void ACSKGameMode::ResetResourcesForPlayers()
{
	ResetPlayerResources(Player1PC, 1);
	ResetPlayerResources(Player2PC, 2);
}

void ACSKGameMode::CollectResourcesForPlayers()
{
	UpdatePlayerResources(Player1PC, 1);
	UpdatePlayerResources(Player2PC, 2);
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

void ACSKGameMode::MovePlayersCastleTo(ACSKPlayerController* Controller, ATile* Tile) const
{
	if (!Tile)
	{
		return;
	}

	// Make sure the controller can actually request a move
	if (Controller && Controller->CanRequestMoveAction())
	{
		ACastle* Castle = Controller->GetCastlePawn();
		check(Castle);

		ATile* Origin = Castle->GetCachedTile();
		if (ensure(Origin) && CanPlayerMoveToTile(Controller, Origin, Tile))
		{
			ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
			check(BoardManager);

			FBoardPath OutBoardPath;
			if (BoardManager->FindPath(Origin, Tile, OutBoardPath, false, MaxTileMovements))
			{

			}
		}
	}
}

bool ACSKGameMode::CanPlayerMoveToTile(ACSKPlayerController* Controller, ATile* From, ATile* To) const
{
	check(Controller);
	check(From != nullptr);
	check(To != nullptr);

	ACSKPlayerState* State = Controller->GetCSKPlayerState();
	if (ensure(State))
	{
		// Get the amount of tiles between both the castle and the requested tile
		int32 Distance = FHexGrid::HexDisplacement(From->GetGridHexValue(), To->GetGridHexValue());

		// Player might have already travelled this turn already
		if (IsCountWithinTileTravelLimits(Distance) && Distance <= State->GetTilesTraversedThisRound())
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE