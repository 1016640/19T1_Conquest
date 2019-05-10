// Fill out your copyright notice in the Description page of Project Settings.

#include "WinnerSequenceActor.h"
#include "CSKGameState.h"
#include "CSKPawn.h"

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

