// Fill out your copyright notice in the Description page of Project Settings.

#include "SpellActor.h"
#include "CSKGameMode.h"

#include "TimerManager.h"
#include "Components/SceneComponent.h"

ASpellActor::ASpellActor()
{
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

void ASpellActor::InitSpellActor(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, ATile* InTargetedTile)
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
	if (HasAuthority() && !bRunning)
	{
		bRunning = true;
		OnActivateSpell();
	}
}

bool ASpellActor::CheckSpellIsCancelled()
{
	bool bWasCancelled = bCancelled;

	if (bCancelled && bRunning)
	{
		FinishSpell();
	}

	return bWasCancelled;
}

void ASpellActor::CancelExecution()
{
	if (bRunning)
	{
		bCancelled = true;
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("Spell actor %s has been cancelled before running"), *GetFName().ToString());
	}
}

void ASpellActor::OnActivateSpell_Implementation()
{
	UE_LOG(LogConquest, Warning, TEXT("Spell Actor Class %s has no implementation for OnActivateSpell. Finishing spell next frame"), *GetName());

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimerForNextTick(this, &ASpellActor::FinishSpell);
}

void ASpellActor::FinishSpell()
{
	if (bRunning)
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			GameMode->NotifyCastSpellFinished(bCancelled);
		}

		bRunning = false;
		bCancelled = false;
	}
}
