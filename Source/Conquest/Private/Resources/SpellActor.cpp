// Fill out your copyright notice in the Description page of Project Settings.

#include "SpellActor.h"
#include "CSKGameMode.h"
#include "CSKGameState.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

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

	bIsPrimarySpell = true;
	bRunning = false;
	bCancelled = false;

	bIsInputBound = false;
	bIsTimerBound = false;

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
	DOREPLIFETIME_CONDITION(ASpellActor, AdditionalMana, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ASpellActor, TargetedTile, COND_InitialOnly);
}

void ASpellActor::InitSpellActor(ACSKPlayerState* InCastingPlayer, USpell* InCastingSpell, int32 InActivationCost, int32 InAdditionalMana, ATile* InTargetedTile)
{
	if (HasAuthority() && !HasActorBegunPlay())
	{
		CastingPlayer = InCastingPlayer;
		CastingSpell = InCastingSpell;
		ActivationCost = InActivationCost;
		AdditionalMana = InAdditionalMana;
		TargetedTile = InTargetedTile;
	}
}

void ASpellActor::BeginExecution(bool bIsPrimeSpell)
{
	if (HasAuthority() && !bRunning)
	{
		bIsPrimarySpell = bIsPrimeSpell;
		bRunning = true;

		OnActivateSpell(bIsPrimeSpell);
	}
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

void ASpellActor::OnActivateSpell_Implementation(bool bIsPrimeSpell)
{
	UE_LOG(LogConquest, Warning, TEXT("Spell Actor Class %s has no implementation for OnActivateSpell. Finishing spell next frame"), *GetName());

	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimerForNextTick(this, &ASpellActor::FinishSpell);
}

void ASpellActor::FinishSpell()
{
	if (bRunning)
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
		if (GameMode)
		{
			GameMode->NotifyCastSpellFinished(this, bCancelled);
		}

		bRunning = false;
		bCancelled = false;
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

void ASpellActor::BindPlayerInput()
{
	if (HasAuthority())
	{
		if (CanSpellContinueExecution() && !bIsInputBound)
		{
			ACSKPlayerController* Controller = CastingPlayer->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.BindDynamic(this, &ASpellActor::BP_CanSelectTileForSpell);
				Controller->CustomOnSelectTile.BindDynamic(this, &ASpellActor::BP_OnTileSelectedForSpell);

				// Notify client to also bind callbacks
				Client_BindPlayerInput(true);

				bIsInputBound = true;
			}
		}
	}
}

void ASpellActor::UnbindPlayerInput()
{
	if (HasAuthority())
	{
		if (bIsInputBound)
		{
			ACSKPlayerController* Controller = CastingPlayer->GetCSKPlayerController();
			if (Controller)
			{
				Controller->CustomCanSelectTile.Unbind();
				Controller->CustomOnSelectTile.Unbind();

				// Notify client to also unbind callbacks
				Client_BindPlayerInput(false);
			}

			bIsInputBound = false;
		}
	}
}

bool ASpellActor::SetCustomTimer(int32 Duration)
{
	if (HasAuthority())
	{
		// We don't check if we have already bound a timer to allow for timers to be reset
		if (CanSpellContinueExecution())
		{
			ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
			if (GameState && GameState->ActivateCustomTimer(Duration))
			{
				GameState->GetCustomTimerFinishedEvent().BindDynamic(this, &ASpellActor::OnCustomTimerFinished);
				bIsTimerBound = true;

				return true;
			}
			else
			{
				UE_LOG(LogConquest, Warning, TEXT("ASpellActor: Failed to set custom timer with duration of %i"), Duration);
			}
		}
	}

	return false;
}

void ASpellActor::ClearCustomTimer()
{
	if (HasAuthority())
	{
		if (bIsTimerBound)
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

void ASpellActor::Client_BindPlayerInput_Implementation(bool bBind)
{
	if (CastingPlayer)
	{
		ACSKPlayerController* Controller = CastingPlayer->GetCSKPlayerController();
		if (CastingPlayer)
		{
			if (bBind)
			{
				EnableInput(Controller);
				Controller->CustomCanSelectTile.BindDynamic(this, &ASpellActor::BP_CanSelectTileForSpell);

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

void ASpellActor::OnCustomTimerFinished(bool bWasSkipped)
{
	// We first clear the custom timer (as blueprints could possibly reset it)
	ClearCustomTimer();

	BP_OnSpellCustomTimerFinished(!bWasSkipped);
}

ASpellActor* ASpellActor::CastSubSpell(TSubclassOf<USpell> InSpell, ATile* InTargetTile, int32 InAdditionalMana, int32 InOverrideCost)
{
	if (!bIsPrimarySpell)
	{
		return nullptr;
	}

	// Game mode only exists on the server
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		ASpellActor* SpellActor = GameMode->CastSubSpellForActiveSpell(InSpell, InTargetTile, InAdditionalMana, InOverrideCost);
		if (SpellActor)
		{
			// Bind callbacks if it's our first sub spell
			if (ActiveSubSpells.Num() == 0)
			{
				GameMode->OnSubSpellFinished.BindDynamic(this, &ASpellActor::OnSubSpellFinished);
			}

			ActiveSubSpells.Add(SpellActor);
		}

		return SpellActor;
	}

	return nullptr;
}

void ASpellActor::OnSubSpellFinished(ASpellActor* FinishedSpell)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ensure(ActiveSubSpells.Contains(FinishedSpell)))
	{
		BP_OnSubSpellFinished(FinishedSpell);
		ActiveSubSpells.Remove(FinishedSpell);

		// Remove callbacks if we have no more spells
		if (ActiveSubSpells.Num() == 0)
		{
			ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
			if (GameMode)
			{
				GameMode->OnSubSpellFinished.Unbind();
			}
		}
	}
}