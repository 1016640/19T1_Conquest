// Fill out your copyright notice in the Description page of Project Settings.

#include "WinnerSequenceActor.h"
#include "CSKGameState.h"
#include "CSKPawn.h"
#include "CSKPlayerState.h"

#include "Castle.h"
#include "Components/StaticMeshComponent.h"
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

void APortalReachedSequenceActor::BeginPlay()
{
	Super::BeginPlay();
	
	ACastle* Castle = WinningPlayer ? WinningPlayer->GetCastle() : nullptr;
	if (Castle)
	{
		UStaticMeshComponent* Mesh = Castle->GetMesh();
		InitialRotation = Mesh->GetComponentRotation();

		// Have local player focus on winners castle
		ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
		if (GameState)
		{
			ACSKPawn* CSKPawn = GameState->GetLocalPlayerPawn();
			if (CSKPawn)
			{
				CSKPawn->TravelToLocation(Castle->GetActorLocation(), 1.f);
			}
		}
	}
	else
	{
		InitialRotation = FRotator::ZeroRotator;
	}
}

void APortalReachedSequenceActor::ConstructTimeline()
{
	Super::ConstructTimeline();

	// Set rotation callback
	if (RotationCurve)
	{
		FOnTimelineFloat RotationCallback;
		RotationCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(APortalReachedSequenceActor, OnRotationInterpUpdated));
		SequenceTimeline.AddInterpFloat(RotationCurve, RotationCallback);
	}

	// Set scaling callback
	if (ScaleCurve)
	{
		FOnTimelineVector ScaleCallback;
		ScaleCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(APortalReachedSequenceActor, OnScaleInterpUpdated));
		SequenceTimeline.AddInterpVector(ScaleCurve, ScaleCallback);
	}
}

void APortalReachedSequenceActor::OnTimelineFinished()
{
	Super::OnTimelineFinished();

	// We hide the players castle to simulate them having transported to another realm
	ACastle* Castle = WinningPlayer ? WinningPlayer->GetCastle() : nullptr;
	if (Castle)
	{
		Castle->SetActorHiddenInGame(true);
	}
}

void APortalReachedSequenceActor::OnRotationInterpUpdated(float Value)
{
	ACastle* Castle = WinningPlayer ? WinningPlayer->GetCastle() : nullptr;
	if (Castle)
	{
		// Rotate around yaw axis
		float Degrees = 360.f * (Value - FMath::TruncToFloat(Value));

		UStaticMeshComponent* Mesh = Castle->GetMesh();
		Mesh->SetWorldRotation(InitialRotation + FRotator(0.f, Degrees, 0.f));		
	}
}

void APortalReachedSequenceActor::OnScaleInterpUpdated(FVector Value)
{
	ACastle* Castle = WinningPlayer ? WinningPlayer->GetCastle() : nullptr;
	if (Castle)
	{
		UStaticMeshComponent* Mesh = Castle->GetMesh();
		Mesh->SetWorldScale3D(Value);
	}
}

ACastleDestroyedSequenceActor::ACastleDestroyedSequenceActor()
{
	MinDelayInterval = 0.4f;
	MaxDelayInterval = 0.6f;
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

void ACastleDestroyedSequenceActor::BeginPlay()
{
	// We need to reset this as clients will have only replicated the initial seed
	RandomStream.Reset();

	Super::BeginPlay();

	// Have local player focus on losers castle
	ACSKGameState* GameState = UConquestFunctionLibrary::GetCSKGameState(this);
	if (GameState)
	{
		ACSKPawn* CSKPawn = GameState->GetLocalPlayerPawn();
		if (CSKPawn && OpponentsCastle)
		{
			CSKPawn->TravelToLocation(OpponentsCastle->GetActorLocation(), 1.f);
		}
	}

	// This will start the interval event loop
	OnIntervalDelayFinished();
}

void ACastleDestroyedSequenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACastleDestroyedSequenceActor, OpponentsCastle, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ACastleDestroyedSequenceActor, RandomStream, COND_InitialOnly);
}

#if WITH_EDITOR
void ACastleDestroyedSequenceActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACastleDestroyedSequenceActor, MinDelayInterval))
	{
		MaxDelayInterval = FMath::Max(MinDelayInterval, MaxDelayInterval);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ACastleDestroyedSequenceActor, MaxDelayInterval))
	{
		MinDelayInterval = FMath::Min(MinDelayInterval, MaxDelayInterval);
	}
}
#endif

void ACastleDestroyedSequenceActor::OnIntervalDelayFinished()
{
	BP_OnIntervalEvent();

	float RandomDelay = RandomStream.FRandRange(MinDelayInterval, MaxDelayInterval);
	
	// Keep cycling back here
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.SetTimer(Handle_IntervalEvent, this, &ACastleDestroyedSequenceActor::OnIntervalDelayFinished, RandomDelay, false);
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