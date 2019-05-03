// Fill out your copyright notice in the Description page of Project Settings.

#include "CoinSequenceActor.h"
#include "CSKPlayerController.h"
#include "CSKGameMode.h"

#include "Coin.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"

ACoinSequenceActor::ACoinSequenceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// We simulate movement on the client
	bReplicates = true;
	bReplicateMovement = false;
	bNetLoadOnClient = true;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(DummyRoot);

	bIsSequenceRunning = false;

	#if WITH_EDITORONLY_DATA
	bSkipSequenceInPIE = false;
	#endif
}

void ACoinSequenceActor::BeginPlay()
{
	Super::BeginPlay();

	// We need to set these here, since we will become view target before we actually start the sequence
	if (Coin)
	{
		FVector StartPosition = GetActorTransform().TransformPosition(FlipCameraLocation);
		Camera->SetWorldLocation(StartPosition);

		FVector Displacement = Coin->GetCoinLocation() - StartPosition;
		FRotator Rotation = FRotationMatrix::MakeFromX(Displacement).Rotator();
		Camera->SetWorldRotation(Rotation);
	}
}

void ACoinSequenceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSequenceRunning && Coin)
	{
		// Move camera first (So Look At uses best values)
		FVector NewLocation = FMath::VInterpTo(Camera->GetComponentLocation(), CameraDesiredLocation, DeltaTime, 5.f);
		Camera->SetWorldLocation(NewLocation);

		// Get the rotation needed for camera to face the coin
		FVector Displacement = Coin->GetCoinLocation() - NewLocation;
		FRotator Rotation = FRotationMatrix::MakeFromX(Displacement).Rotator();
		Camera->SetWorldRotation(Rotation);

		// Sequence technically concludes once we reach the final camera location
		/*if (FVector::PointsAreNear(NewLocation, CameraDesiredLocation, 1.f))
		{
			SetActorTickEnabled(false);
		}*/
	}
}

void ACoinSequenceActor::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Camera->GetCameraView(DeltaTime, OutResult);
}

void ACoinSequenceActor::StartCoinSequence()
{
	if (HasAuthority())
	{
		if (CanActivateCoinSequence())
		{
			// We only handle this on the server
			Coin->OnCoinFlipComplete.AddDynamic(this, &ACoinSequenceActor::ServerHandleCoinFlipFinished);
			Multi_StartCoinFlip();
		}
	}
}

void ACoinSequenceActor::FinishCoinSequence()
{
	if (HasAuthority())
	{
		if (Coin)
		{
			Coin->OnCoinFlipComplete.RemoveDynamic(this, &ACoinSequenceActor::ServerHandleCoinFlipFinished);
			Multi_FinishCoinFlip();
		}
	}
}

bool ACoinSequenceActor::CanActivateCoinSequence() const
{
	if (Coin)
	{
		#if WITH_EDITORONLY_DATA
		return !bSkipSequenceInPIE;
		#else
		return true;
		#endif
	}

	return false;
}

void ACoinSequenceActor::Multi_StartCoinFlip_Implementation()
{
	if (Coin)
	{
		Coin->OnCoinFlipComplete.AddDynamic(this, &ACoinSequenceActor::ClientHandleCoinFlipFinished);

		// Desired location in world space
		CameraDesiredLocation = GetActorTransform().TransformPosition(FlipCameraLocation);
		Camera->SetWorldLocation(CameraDesiredLocation);

		SetActorTickEnabled(true);

		// Locally simulate the coin flip
		Coin->Flip();
		bIsSequenceRunning = true;
	}
}

void ACoinSequenceActor::Multi_FinishCoinFlip_Implementation()
{
	if (Coin)
	{
		Coin->OnCoinFlipComplete.RemoveDynamic(this, &ACoinSequenceActor::ClientHandleCoinFlipFinished);
		SetActorTickEnabled(false);

		bIsSequenceRunning = false;
	}
}

void ACoinSequenceActor::ServerHandleCoinFlipFinished(bool bHeads)
{
	ACSKGameMode* GameMode = UConquestFunctionLibrary::GetCSKGameMode(this);
	if (GameMode)
	{
		GameMode->OnStartingPlayerDecided(Coin->GetWinnersPlayerID());
	}

	Coin->OnCoinFlipComplete.RemoveDynamic(this, &ACoinSequenceActor::ServerHandleCoinFlipFinished);
}

void ACoinSequenceActor::ClientHandleCoinFlipFinished(bool bHeads)
{
	CameraDesiredLocation = GetActorTransform().TransformPosition(FocusCameraLocation);
	Coin->OnCoinFlipComplete.RemoveDynamic(this, &ACoinSequenceActor::ClientHandleCoinFlipFinished);
}

