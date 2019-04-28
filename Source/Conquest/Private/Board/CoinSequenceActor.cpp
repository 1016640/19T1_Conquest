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
		FVector Displacement = Coin->GetCoinLocation() - Camera->GetComponentLocation();
		FRotator Rotation = FRotationMatrix::MakeFromX(Displacement).Rotator();
		Camera->SetWorldRotation(Rotation);
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
		if (Coin)
		{
			Coin->OnCoinFlipComplete.AddDynamic(this, &ACoinSequenceActor::ServerHandleCoinFlipFinished);
			Multi_StartCoinFlip();
		}
	}
}

void ACoinSequenceActor::Multi_StartCoinFlip_Implementation()
{
	if (Coin)
	{
		Coin->OnCoinFlipComplete.AddDynamic(this, &ACoinSequenceActor::ClientHandleCoinFlipFinished);
		Coin->Flip();

		CameraDesiredLocation = GetActorTransform().TransformPosition(FlipCameraLocation);
		Camera->SetWorldLocation(CameraDesiredLocation);

		bIsSequenceRunning = true;
		SetActorTickEnabled(true);
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

