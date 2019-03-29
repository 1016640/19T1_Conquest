// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPawn.h"
#include "CSKPlayerController.h"

#include "CSKPawnMovement.h"
#include "CSKSpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"

ACSKPawn::ACSKPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	bFindCameraComponentWhenViewTarget = true;

	bReplicates = true;
	bReplicateMovement = true;
	bAlwaysRelevant = false;
	bOnlyRelevantToOwner = true;

	NetUpdateFrequency = 25.f;
	NetPriority = 0.25f;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SetRootComponent(SphereCollision);
	SphereCollision->InitSphereRadius(10.f);
	SphereCollision->SetCollisionProfileName(TEXT("Pawn"));

	CameraBoom = CreateDefaultSubobject<UCSKSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(SphereCollision);
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));
	CameraBoom->InitTargetArmLengths(1000.f, 4000.f);
	CameraBoom->TargetArmLength = 2000.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->SetRelativeRotation(FRotator(10.f, 0.f, 0.f));
	Camera->bUsePawnControlRotation = false;

	PawnMovement = CreateDefaultSubobject<UCSKPawnMovement>(TEXT("MovementComp"));
	PawnMovement->SetUpdatedComponent(SphereCollision);
	PawnMovement->MaxSpeed = 2000.f;
	PawnMovement->Acceleration = 2000.f;
	PawnMovement->Deceleration = 4000.f;
	PawnMovement->TurningBoost = 10.f;
}

void ACSKPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	// Mobility
	PlayerInputComponent->BindAxis("MoveForward", this, &ACSKPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACSKPawn::MoveRight);

	// Zoom
	PlayerInputComponent->BindAxis("Zoom", this, &ACSKPawn::ZoomCamera);
}

void ACSKPawn::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		// We only want to move based on the view yaw (as the camera is mostly likely facing down)
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		// Get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACSKPawn::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		// We only want to move based on the view yaw (as the camera is mostly likely facing down)
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		// Get right vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ACSKPawn::ZoomCamera(float Value)
{
	// TODO: Replace the fixed 300.f with a User Settings option
	CameraBoom->AddDesiredArmLength(300.f * Value);
}
