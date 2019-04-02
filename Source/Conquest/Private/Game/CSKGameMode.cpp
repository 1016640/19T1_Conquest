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
#include "Tile.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "CSKGameMode"

ACSKGameMode::ACSKGameMode()
{
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

void ACSKGameMode::PostLogin(APlayerController* NewPlayer)
{
	// We need to set which player this is before continuing the login
	{
		if (Player1PC)
		{
			// We should only ever have two players
			if (ensure(!Player2PC))
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
		// End match if still active
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

#undef LOCTEXT_NAMESPACE