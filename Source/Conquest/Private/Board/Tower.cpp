// Fill out your copyright notice in the Description page of Project Settings.

#include "Tower.h"
#include "Tile.h"
#include "BoardManager.h"
#include "CSKGameMode.h"
#include "CSKGameState.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "HealthComponent.h"
#include "TowerConstructionData.h"
#include "Components/StaticMeshComponent.h"

ATower::ATower()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	bReplicateMovement = false;
	bOnlyRelevantToOwner = false;

	// Actors aren't relevant when hidden, this will result in board piece
	// placement in ATile passing null when placing this tower on the client.
	// We need the PlacedOnTile event to fire so we move client side
	bAlwaysRelevant = true;		

	OwnerPlayerState = nullptr;
	CachedTile = nullptr;
	bIsLegendaryTower = false;
	bGivesCollectionPhaseResources = false;
	bWantsActionDuringEndRoundPhase = false;
	EndRoundPhaseActionPriority = 0;
	BuildSequenceUndergroundScale = 1.5f;

	bIsRunningEndRoundAction = false;
	bIsInputBound = false;
	bIsTimerBound = false;

	// TODO: We might change this to skeletal meshes later
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);

	HealthTracker = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthTracker->InitHealth(5, 5);
}

void ATower::SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState)
{
	if (HasAuthority())
	{
		OwnerPlayerState = InPlayerState;
	}
}

void ATower::PlacedOnTile(ATile* Tile)
{
	CachedTile = Tile;
	StartBuildSequence();
}

void ATower::OnHoverStart()
{
	BP_OnHoveredByPlayer();
}

void ATower::OnHoverFinish()
{
	BP_OnUnhoveredByPlayer();
}

void ATower::GetBoardPieceUIData(FBoardPieceUIData& OutUIData) const
{
	if (ConstructData)
	{
		OutUIData.Name = FText::FromName(ConstructData->TowerName);
		
		// Replace all new lines with spaces
		FString DescriptionString = ConstructData->TowerDescription.ToString();
		DescriptionString = DescriptionString.Replace(TEXT("\r\n"), TEXT(" "));
		OutUIData.Description = FText::FromString(DescriptionString);
	}
}

void ATower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// We only perform this during the build sequence, we have
	// to check as some derived buildings might also use tick
	if (bIsRunningBuildSequence)
	{
		// TODO: make config variable
		static const float InterpSpeed = 2.f;

		FVector CurLocation = GetActorLocation();
		FVector TarLocation = CachedTile->GetActorLocation();

		float CurZ = CurLocation.Z;
		float TarZ = TarLocation.Z;
		float NewZ = FMath::FInterpTo(CurZ, TarZ, DeltaTime, InterpSpeed);

		// Are we now ontop of tile?
		if (FMath::IsNearlyEqual(NewZ, TarZ, 1.f))
		{
			// Snap ourselves to our target tile
			SetActorLocation(TarLocation);

			FinishBuildSequence();
		}
		else
		{
			// Move ourselves towards goal
			CurLocation.Z = NewZ;
			SetActorLocation(CurLocation);
		}
	}
}

void ATower::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATower, ConstructData, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ATower, OwnerPlayerState, COND_InitialOnly);
	DOREPLIFETIME(ATower, bIsRunningEndRoundAction);
}

void ATower::ExecuteEndRoundPhaseAction()
{
	if (!bIsRunningEndRoundAction && HasAuthority())
	{
		bIsRunningEndRoundAction = true;
		StartEndRoundAction();

		// We want RunnedEndRoundAction to replicated as soon as possible
		ForceNetUpdate();
	}
}

void ATower::StartEndRoundAction_Implementation()
{
	FinishEndRoundAction();
}

void ATower::FinishEndRoundAction()
{
	if (bIsRunningEndRoundAction && HasAuthority())
	{
		// Unbind any callbacks we may have bound
		if (bIsInputBound)
		{
			UnbindPlayerInput();
		}

		// Stop the timer we may have bound
		if (bIsTimerBound)
		{
			ClearCustomTimer();
		}

		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		check(GameMode);
		
		GameMode->NotifyEndRoundActionFinished(this);	
		bIsRunningEndRoundAction = false;

		// We want RunnedEndRoundAction to replicated as soon as possible
		ForceNetUpdate();
	}
}

void ATower::StartBuildSequence()
{
	if (!bIsRunningBuildSequence)
	{
		// Start off under the board, our sequence involves us moving on top of it
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (BoardManager)
		{
			BoardManager->MoveBoardPieceUnderBoard(this, BuildSequenceUndergroundScale);
		}

		// Start the building sequence
		SetActorHiddenInGame(false);
		SetActorTickEnabled(true);

		bIsRunningBuildSequence = true;
		BP_OnStartBuildSequence();
	}
}

void ATower::FinishBuildSequence()
{
	if (bIsRunningBuildSequence)
	{
		// Only needed to tick to move ourselves into place
		SetActorTickEnabled(false);

		bIsRunningBuildSequence = false;
		BP_OnFinishBuildSequence();

		if (HasAuthority())
		{
			OnBuildSequenceComplete.Broadcast();
		}
	}
}

bool ATower::WantsCollectionPhaseEvent_Implementation() const
{
	return bGivesCollectionPhaseResources;
}

bool ATower::WantsEndRoundPhaseEvent_Implementation() const
{
	return bWantsActionDuringEndRoundPhase; 
}

void ATower::BindPlayerInput()
{
	if (HasAuthority())
	{
		if (bIsRunningEndRoundAction && !bIsInputBound)
		{
			ACSKPlayerController* Controller = OwnerPlayerState->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.BindDynamic(this, &ATower::BP_CanSelectTileForAction);
				Controller->CustomOnSelectTile.BindDynamic(this, &ATower::BP_OnTileSelectedForAction);

				// Notify client to also bind callbacks
				Client_BindPlayerInput(true);

				bIsInputBound = true;
			}
		}
	}
}

void ATower::UnbindPlayerInput()
{
	if (HasAuthority())
	{
		if (bIsRunningEndRoundAction && bIsInputBound)
		{
			ACSKPlayerController* Controller = OwnerPlayerState->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.Unbind();
				Controller->CustomOnSelectTile.Unbind();

				// Notify client to also unbind callbacks
				Client_BindPlayerInput(false);

				bIsInputBound = false;
			}
		}
	}
}

bool ATower::SetCustomTimer(int32 Duration)
{
	if (HasAuthority())
	{
		// We don't check if we have already bound a timer to allow for timers to be reset
		if (bIsRunningEndRoundAction)
		{
			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState && GameState->ActivateCustomTimer(Duration))
			{
				GameState->GetCustomTimerFinishedEvent().BindDynamic(this, &ATower::OnCustomTimerFinished);
				bIsTimerBound = true;

				return true;
			}
			else
			{
				UE_LOG(LogConquest, Warning, TEXT("ATower: Failed to set custom timer with duration of %i"), Duration);
			}
		}
	}

	return false;
}

void ATower::ClearCustomTimer()
{
	if (HasAuthority())
	{
		if (bIsRunningEndRoundAction && bIsTimerBound)
		{
			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState)
			{
				GameState->DeactivateCustomTimer();
				GameState->GetCustomTimerFinishedEvent().Unbind();
			}

			bIsTimerBound = false;
		}
	}
}

void ATower::Client_BindPlayerInput_Implementation(bool bBind)
{
	if (OwnerPlayerState)
	{
		ACSKPlayerController* Controller = OwnerPlayerState->GetCSKPlayerController();
		if (Controller)
		{
			if (bBind)
			{
				EnableInput(Controller);
				Controller->CustomCanSelectTile.BindDynamic(this, &ATower::BP_CanSelectTileForAction);
				
				// Player needs to be able to move
				Controller->SetIgnoreMoveInput(false);
				Controller->SetCanSelectTile(true);
				
			}
			else
			{
				DisableInput(Controller);
				Controller->CustomCanSelectTile.Unbind();

				// Player should no longer be able to move
				Controller->SetIgnoreMoveInput(true);
				Controller->SetCanSelectTile(false);
			}
		}
	}
}

void ATower::OnCustomTimerFinished(bool bWasSkipped)
{
	// We first clear the custom timer (as blueprints could possibly reset it)
	ClearCustomTimer();

	BP_OnActionCustomTimerFinished(!bWasSkipped);
}

