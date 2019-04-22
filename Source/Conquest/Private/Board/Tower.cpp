// Fill out your copyright notice in the Description page of Project Settings.

#include "Tower.h"
#include "Tile.h"
#include "BoardManager.h"
#include "CSKGameMode.h"
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

	bIsRunningEndRoundAction = false;
	bIsCustomTileCallbacksBound = false;

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

void ATower::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ATower, ConstructData, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ATower, OwnerPlayerState, COND_InitialOnly);
}

void ATower::ExecuteEndRoundPhaseAction()
{
	if (!bIsRunningEndRoundAction && HasAuthority())
	{
		bIsRunningEndRoundAction = true;
		StartEndRoundAction();
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
		if (bIsCustomTileCallbacksBound)
		{
			UnbindPlayerTileSelectionCallbacks();
		}

		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		check(GameMode);
		
		GameMode->NotifyEndRoundActionFinished(this);	
		bIsRunningEndRoundAction = false;
	}
}

void ATower::StartBuildSequence()
{
	// Start off under the board, our sequence involves us moving on top of it
	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		BoardManager->MoveBoardPieceUnderBoard(this);
	}

	// Start the building sequence
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	BP_OnStartBuildSequence();
}

void ATower::FinishBuildSequence()
{
	// Only needed to tick to move ourselves into place
	SetActorTickEnabled(false);

	BP_OnFinishBuildSequence();

	if (HasAuthority())
	{
		OnBuildSequenceComplete.Broadcast();
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

void ATower::BindPlayerTileSelectionCallbacks()
{
	if (HasAuthority())
	{
		if (bIsRunningEndRoundAction && !bIsCustomTileCallbacksBound)
		{
			ACSKPlayerController* Controller = OwnerPlayerState->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.BindDynamic(this, &ATower::BP_CanSelectTileForAction);
				Controller->CustomOnSelectTile.BindDynamic(this, &ATower::BP_OnTileSelectedForAction);

				// Notify client to also unbind callbacks
				Client_BindPlayerTileSelectionCallbacks(true);

				bIsCustomTileCallbacksBound = true;
			}
		}
	}
}

void ATower::UnbindPlayerTileSelectionCallbacks()
{
	if (HasAuthority())
	{
		if (bIsRunningEndRoundAction && bIsCustomTileCallbacksBound)
		{
			ACSKPlayerController* Controller = OwnerPlayerState->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.Unbind();
				Controller->CustomOnSelectTile.Unbind();

				// Notify client to also bind callbacks
				Client_BindPlayerTileSelectionCallbacks(false);

				bIsCustomTileCallbacksBound = false;
			}
		}
	}
}

void ATower::Client_BindPlayerTileSelectionCallbacks_Implementation(bool bBind)
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
