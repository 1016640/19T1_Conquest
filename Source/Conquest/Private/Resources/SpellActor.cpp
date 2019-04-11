// Fill out your copyright notice in the Description page of Project Settings.

#include "SpellActor.h"
#include "CSKGameMode.h"

#include "TimerManager.h"
#include "Components/SceneComponent.h"

// Sets default values
ASpellActor::ASpellActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Spells are always relevant when executing
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;

	CastingPlayer = nullptr;
	CastingSpell = nullptr;
	ActivationCost = 0;
	TargetedTile = nullptr;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->SetMobility(EComponentMobility::Movable);
}

void ASpellActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASpellActor, CastingPlayer, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASpellActor, CastingSpell, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASpellActor, ActivationCost, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASpellActor, TargetedTile, COND_InitialOnly);
}

void ASpellActor::SetActivationInfo(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, ATile* InTargetedTile)
{
	if (HasAuthority() && !HasActorBegunPlay())
	{
		CastingPlayer = InCastingPlayer;
		CastingSpell = InCastingSpell;
		ActivationCost = InActivationCost;
		TargetedTile = InTargetedTile;
	}
}

void ASpellActor::BeginExecution()
{
	if (HasAuthority() && !bIsRunning)
	{
		bIsRunning = true;
		OnActivateSpell();
	}
}

void ASpellActor::OnActivateSpell_Implementation()
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimerForNextTick(this, &ASpellActor::FinishSpell);
}

void ASpellActor::FinishSpell()
{
	if (bIsRunning)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			GameMode->NotifyCastSpellFinished();
		}

		bIsRunning = false;
	}
}
