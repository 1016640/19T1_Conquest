// Fill out your copyright notice in the Description page of Project Settings.

#include "Castle.h"
#include "CastleAIController.h"
#include "CSKPlayerState.h"

#include "HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

#define LOCTEXT_NAMESPACE "Castle"

ACastle::ACastle()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bReplicateMovement = true;
	bOnlyRelevantToOwner = false;

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = ACastleAIController::StaticClass();

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->SetMobility(EComponentMobility::Movable);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(DummyRoot);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
	PawnMovement->SetUpdatedComponent(DummyRoot);
	PawnMovement->MaxSpeed = 800.f;
	PawnMovement->Acceleration = 2000.f;
	PawnMovement->Deceleration = 2000.f;
	PawnMovement->TurningBoost = 0.f;

	HealthTracker = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthTracker->InitHealth(40, 40);

	OwnerPlayerState = nullptr;
	CachedTile = nullptr;
}

void ACastle::SetBoardPieceOwnerPlayerState(ACSKPlayerState* InPlayerState)
{
	if (HasAuthority())
	{
		OwnerPlayerState = InPlayerState;
	}
}

void ACastle::PlacedOnTile(ATile* Tile)
{
	CachedTile = Tile;
	BP_OnPlacedOnTile(CachedTile);
}

void ACastle::RemovedOffTile()
{
	BP_OnRemovedFromTile(CachedTile);
	CachedTile = nullptr;
}

void ACastle::OnHoverStart()
{
	BP_OnHoveredByPlayer();
}

void ACastle::OnHoverFinish()
{
	BP_OnUnhoveredByPlayer();
}

void ACastle::GetBoardPieceUIData(FBoardPieceUIData& OutUIData) const
{
	if (OwnerPlayerState)
	{
		FFormatNamedArguments Args;
		Args.Add("PlayerName", FText::FromString(OwnerPlayerState->GetPlayerName()));
		OutUIData.Name = FText::Format(LOCTEXT("CastleUIName", "{PlayerName}s Castle"), Args);
	}
}

void ACastle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACastle, OwnerPlayerState, COND_InitialOnly);
}

#undef LOCTEXT_NAMESPACE