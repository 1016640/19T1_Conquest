// Fill out your copyright notice in the Description page of Project Settings.

#include "Castle.h"
#include "CastleAIController.h"

#include "Components/SkeletalMeshComponent.h"

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
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

