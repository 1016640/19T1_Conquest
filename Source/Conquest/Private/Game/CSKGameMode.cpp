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
}

void ACSKGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CanStartMatch())
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

	EnterMatchState(ECSKMatchState::EnteringMap);

	// Callbacks required
	FGameDelegates& GameDelegates = FGameDelegates::Get();
	GameDelegates.GetHandleDisconnectDelegate().AddUObject(this, &ACSKGameMode::OnDisconnect);
}

void ACSKGameMode::StartPlay()
{
	if (MatchState == ECSKMatchState::EnteringMap)
	{
		EnterMatchState(ECSKMatchState::PreMatchWait);
	}

	if (MatchState == ECSKMatchState::PreMatchWait)
	{
		StartMatch();
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
		const ATile* PortalTile = BoardManager->GetPlayerPortalTile(PlayerID);
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
		//UE_LOG(LogConquest, Log, TEXT("Entering Match State: %s"), *UEnum::GetValueAsString<ECSKMatchState>(TEXT("ECSKMatchState"), NewState));

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
	if (CanStartMatch())
	{
		EnterMatchState(ECSKMatchState::InProgress);
	}
}

void ACSKGameMode::EndMatch()
{
	if (CanEndMatch())
	{
		EnterMatchState(ECSKMatchState::PostMatchWait);
	}
}

void ACSKGameMode::AbortMatch()
{
	EnterMatchState(ECSKMatchState::Aborted);
}

bool ACSKGameMode::CanStartMatch_Implementation() const
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

bool ACSKGameMode::CanEndMatch_Implementation() const
{
	// TODO: Check match state
	return false;
}

bool ACSKGameMode::IsMatchInProgress() const
{
	return MatchState == ECSKMatchState::InProgress;
}

bool ACSKGameMode::HasMatchFinished() const
{
	return 
		MatchState == ECSKMatchState::PostMatchWait	||
		MatchState == ECSKMatchState::LeavingMap	||
		MatchState == ECSKMatchState::Aborted;
}

void ACSKGameMode::OnWaitingForPlayers()
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

	AWorldSettings* WorldSettings = GetWorldSettings();
	WorldSettings->NotifyBeginPlay();
	WorldSettings->NotifyMatchStarted();
}

void ACSKGameMode::OnMatchFinished()
{
}

void ACSKGameMode::OnPlayersLeaving()
{
}

void ACSKGameMode::OnMatchAbort()
{
}

void ACSKGameMode::HandleStateChange(ECSKMatchState OldState, ECSKMatchState NewState)
{
	switch (NewState)
	{
		case ECSKMatchState::EnteringMap:
		{
			break;
		}
		case ECSKMatchState::PreMatchWait:
		{
			OnWaitingForPlayers();
			break;
		}
		case ECSKMatchState::InProgress:
		{
			OnMatchStart();
			break;
		}
		case ECSKMatchState::PostMatchWait:
		{
			OnMatchFinished();
			break;
		}
		case ECSKMatchState::LeavingMap:
		{
			OnPlayersLeaving();
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

#undef LOCTEXT_NAMESPACE