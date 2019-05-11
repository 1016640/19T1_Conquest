// Fill out your copyright notice in the Description page of Project Settings.

#include "WinnerSequenceActor.h"
#include "CSKGameState.h"
#include "CSKPawn.h"
#include "CSKPlayerState.h"

#include "Castle.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AWinnerSequenceActor::AWinnerSequenceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
}

void AWinnerSequenceActor::BeginPlay()
{
	Super::BeginPlay();
	
	SequenceTimeline.Play();
}

void AWinnerSequenceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// We want to ignore the dilation being applied (so the
	// duration set by the curve is the duration of the sequence)
	DeltaTime /= GetActorTimeDilation();

	SequenceTimeline.TickTimeline(DeltaTime);
}

void AWinnerSequenceActor::PostInitProperties()
{
	Super::PostInitProperties();

	ConstructTimeline();
}

void AWinnerSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWinnerSequenceActor, WinningPlayer, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AWinnerSequenceActor, WinCondition, COND_InitialOnly);
}

void AWinnerSequenceActor::InitSequenceActor(ACSKPlayerState* InWinningPlayer, ECSKMatchWinCondition InWinCondition)
{
	if (HasAuthority() && !HasActorBegunPlay())
	{
		WinningPlayer = InWinningPlayer;
		WinCondition = InWinCondition;
	}
}

void AWinnerSequenceActor::ConstructTimeline()
{
	SequenceTimeline = FTimeline();

	// Set dilation callback
	if (DilationCurve)
	{
		if (HasAuthority())
		{
			FOnTimelineFloat DilationCallback;
			DilationCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AWinnerSequenceActor, OnDilationInterpUpdated));
			SequenceTimeline.AddInterpFloat(DilationCurve, DilationCallback);
		}
	}

	// Set finished callback
	FOnTimelineEvent FinishedCallback;
	FinishedCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AWinnerSequenceActor, OnTimelineFinished));
	SequenceTimeline.SetTimelineFinishedFunc(FinishedCallback);
}

void AWinnerSequenceActor::OnDilationInterpUpdated(float Value)
{
	if (HasAuthority())
	{
		AWorldSettings* WorldSettings = GetWorldSettings();
		if (WorldSettings)
		{
			if (Value > 0.f)
			{
				WorldSettings->MatineeTimeDilation = Value;
				WorldSettings->ForceNetUpdate();
			}
		}
	}
}

void AWinnerSequenceActor::OnTimelineFinished()
{
	if (HasAuthority())
	{
		// Be sure to restore dilation
		AWorldSettings* WorldSettings = GetWorldSettings();
		if (WorldSettings)
		{
			WorldSettings->MatineeTimeDilation = 1.f;
			WorldSettings->ForceNetUpdate();
		}

		OnSequenceFinished.ExecuteIfBound();
	}

	Destroy();
}

void ACastleDestroyedSequenceActor::InitSequenceActor(ACSKPlayerState* InWinningPlayer, ECSKMatchWinCondition InWinCondition)
{
	if (HasAuthority() && !HasActorBegunPlay())
	{
		Super::InitSequenceActor(InWinningPlayer, InWinCondition);

		// Get the opponents castle
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (ensure(GameState))
		{
			ACSKPlayerState* LosingPlayer = GameState->GetOpposingPlayerState(InWinningPlayer);
			if (ensure(LosingPlayer))
			{
				OpponentsCastle = LosingPlayer->GetCastle();
			}
		}

		// Generate a random seed (will be synced amongst all remote instances)
		{
			RandomStream.GenerateNewSeed();
		}
	}
}

void ACastleDestroyedSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACastleDestroyedSequenceActor, OpponentsCastle, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ACastleDestroyedSequenceActor, RandomStream, COND_InitialOnly);
}

FVector ACastleDestroyedSequenceActor::GetPointAroundCastle(const FVector& Bounds) const
{
	FVector Origin = OpponentsCastle ? OpponentsCastle->GetActorLocation() : FVector::ZeroVector;
	FBox Box = FBox::BuildAABB(Origin, Bounds);
	
	// We have to mimic FMath::RandPointInBox as it doesn't take in a custom seed
	return FVector(
		RandomStream.FRandRange(Box.Min.X, Box.Max.X),
		RandomStream.FRandRange(Box.Min.Y, Box.Max.Y),
		RandomStream.FRandRange(Box.Min.Z, Box.Max.Z));
}
