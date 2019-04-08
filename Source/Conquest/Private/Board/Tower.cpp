// Fill out your copyright notice in the Description page of Project Settings.

#include "Tower.h"
#include "Tile.h"
#include "BoardManager.h"

#include "Components/StaticMeshComponent.h"

ATower::ATower()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	OwnerPlayerState = nullptr;
	CachedTile = nullptr;
	bIsLegendaryTower = false;

	// TODO: We might change this to skeletal meshes later
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetSimulatePhysics(false);
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

void ATower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// TODO: make config variable
	static const float InterpSpeed = 6.f;

	FVector CurLocation = GetActorLocation();
	FVector TarLocation = GetTargetLocation();

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

	DOREPLIFETIME_CONDITION(ATower, OwnerPlayerState, COND_InitialOnly);
}

void ATower::StartBuildSequence()
{
	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		BoardManager->MoveBoardPieceUnderBoard(this);
	}

	// Start the building sequence
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void ATower::FinishBuildSequence()
{
	if (HasAuthority())
	{
		// TODO: notify game mode
	}

	// Only needed to tick to move ourselves into place
	SetActorTickEnabled(false);
}
