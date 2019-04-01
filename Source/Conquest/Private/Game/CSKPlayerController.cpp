// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerController.h"
#include "CSKGameState.h"
#include "CSKLocalPlayer.h"
#include "CSKPlayerState.h"
#include "BoardManager.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

#include "Kismet/KismetSystemLibrary.h"

ACSKPlayerController::ACSKPlayerController()
{
	bShowMouseCursor = true;
}

void ACSKPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HoveredTile)
	{
		HoveredTile->bHighlightTile = false;
	}

	// TODO: Should perform this only client side
	HoveredTile = GetTileUnderMouse();
	if (HoveredTile)
	{
		HoveredTile->bHighlightTile = true;
		UKismetSystemLibrary::PrintString(this, FString("Hovering over tile with hex: ") + HoveredTile->GetGridHexValue().ToString(), true, false, FLinearColor::Green, 0.f);
	}
	else
	{
		UKismetSystemLibrary::PrintString(this, FString("No Tile hovered"), true, false, FLinearColor::Green, 0.f);
	}
}

void ACSKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	// Selection
	InputComponent->BindAction("Select", IE_Pressed, this, &ACSKPlayerController::SelectTile);
}

ACSKPlayerState* ACSKPlayerController::GetCSKPlayerState() const
{
	return GetPlayerState<ACSKPlayerState>();
}

ATile* ACSKPlayerController::GetTileUnderMouse() const
{
	// Local player will only exist if we are a client
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManger(this);
		if (!BoardManager)
		{
			return nullptr;
		}
		
		FVector2D MousePosition;
		if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
		{
			FVector Position;
			FVector Direction;
			if (UGameplayStatics::DeprojectScreenToWorld(this, MousePosition, Position, Direction))
			{
				FVector End = Position + Direction * 10000.f;
				return BoardManager->TraceBoard(Position, End);
			}
		}
	}

	return nullptr;
}

void ACSKPlayerController::SelectTile()
{
	if (HoveredTile)
	{
		UKismetSystemLibrary::PrintString(this, FString("Selected tile with hex: ") + HoveredTile->GetGridHexValue().ToString(), true, false, FLinearColor::Red, 5.f);
	}
}
