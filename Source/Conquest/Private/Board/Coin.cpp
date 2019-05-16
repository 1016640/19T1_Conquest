// Fill out your copyright notice in the Description page of Project Settings.

#include "Coin.h"
#include "CSKGameMode.h"
#include "Components/StaticMeshComponent.h"

ACoin::ACoin()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// We replicate, but simulate movement on every client
	bReplicates = true;
	bReplicateMovement = false;
	bNetLoadOnClient = true;

	// We use this as the base for the coin. We will move and rotate the coin locally to this
	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(DummyRoot);
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FlipHeight = 800.f;
}

void ACoin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ensure(IsCoinFlipping());	
	FlipTimeline.TickTimeline(DeltaTime);

	// We want to determine a winner after playing half of the sequence (but not every frame after)
	if (!bWinnerDetermined && HasHalfOfTimelinePlayed())
	{
		ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
		if (GameMode)
		{
			bLandingOnHeads = GameMode->GenerateCoinFlipWinner();
		}

		bWinnerDetermined = true;
	}
}

void ACoin::PostInitProperties()
{
	Super::PostInitProperties();

	ConstructTimeline();
}

void ACoin::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACoin, bLandingOnHeads);
}

void ACoin::Flip()
{
	bWinnerDetermined = false;

	// Reset mesh
	Mesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	
	FlipTimeline.PlayFromStart();
	SetActorTickEnabled(true);
}

bool ACoin::IsCoinFlipping() const
{
	return FlipTimeline.IsPlaying();
}

int32 ACoin::GetWinnersPlayerID() const
{
	if (bWinnerDetermined)
	{
		return bLandingOnHeads ? 0 : 1;
	}
	
	return -1;
}

void ACoin::ConstructTimeline()
{
	FlipTimeline = FTimeline();

	// Set location callback
	if (LocationCurve)
	{
		FOnTimelineFloat LocationCallback;
		LocationCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ACoin, OnLocationInterpUpdated));
		FlipTimeline.AddInterpFloat(LocationCurve, LocationCallback);
	}

	// Set rotation callback
	if (RotationCurve)
	{
		FOnTimelineFloat RotationCallback;
		RotationCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ACoin, OnRotationInterpUpdated));
		FlipTimeline.AddInterpFloat(RotationCurve, RotationCallback);
	}

	// Set finished callback
	FOnTimelineEvent FinishedCallback;
	FinishedCallback.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ACoin, OnFlipFinished));
	FlipTimeline.SetTimelineFinishedFunc(FinishedCallback);
}

bool ACoin::HasHalfOfTimelinePlayed() const
{
	return FlipTimeline.GetPlaybackPosition() >= FlipTimeline.GetTimelineLength() * .5f;
}

void ACoin::OnLocationInterpUpdated(float Value)
{
	// Move along vertical axis
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, FlipHeight * Value));
}

void ACoin::OnRotationInterpUpdated(float Value)
{
	// Add an additional 180 if we should be landing on tails
	if (bWinnerDetermined && !bLandingOnHeads)
	{
		Value -= 0.5f;
	}

	// Rotate around pitch axis
	float Degrees = 360.f * (Value - FMath::TruncToFloat(Value));

	Mesh->SetRelativeRotation(FRotator(Degrees, 0.f, 0.f));
}

void ACoin::OnFlipFinished()
{
	SetActorTickEnabled(false);

	// Forcefully set our transform to correct settings
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	Mesh->SetRelativeRotation(FRotator(bLandingOnHeads ? 0.f : 180.f, 0.f, 0.f));

	OnCoinFlipComplete.Broadcast(bLandingOnHeads);

	UE_LOG(LogConquest, Log, TEXT("Coin Flip has finished with Player %i determined as the Winner"), GetWinnersPlayerID() + 1);
}

FVector ACoin::GetCoinLocation() const
{
	return Mesh->GetComponentLocation();
}

void ACoin::Multi_SetupCoin_Implementation()
{
	BP_SetupCoin();
}
