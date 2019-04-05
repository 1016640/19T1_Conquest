// Fill out your copyright notice in the Description page of Project Settings.

#include "Castle.h"
#include "CastleAIController.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ACastle::ACastle()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bReplicateMovement = true;
	bOnlyRelevantToOwner = false;

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = ACastleAIController::StaticClass();

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
	PawnMovement->SetUpdatedComponent(Mesh);
	PawnMovement->MaxSpeed = 800.f;
	PawnMovement->Acceleration = 2000.f;
	PawnMovement->Deceleration = 2000.f;
	PawnMovement->TurningBoost = 0.f;

	CachedTile = nullptr;
}

void ACastle::PlacedOnTile(ATile* Tile)
{
	CachedTile = Tile;
}

void ACastle::RemovedOffTile()
{
	CachedTile = nullptr;
}
