// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerController.h"
#include "CSKGameState.h"
#include "CSKLocalPlayer.h"
#include "CSKPlayerState.h"
#include "BoardManager.h"
#include "Castle.h"
#include "CastleAIController.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

ACSKPlayerController::ACSKPlayerController()
{
	bShowMouseCursor = true;
}

void ACSKPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocalPlayerController())
	{
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

		if (TestBoardPath.IsValid())
		{
			const auto& t = TestBoardPath.Path;
			for (int32 i = 0; i < t.Num() - 1;)
			{
				FVector Start = t[i]->GetActorLocation();
				FVector End = t[++i]->GetActorLocation();

				DrawDebugLine(GetWorld(), Start, End, FColor::White, false, -1.f, 5, 5.f);
			}
		}
	}
}

void ACSKPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	// Selection
	InputComponent->BindAction("Select", IE_Pressed, this, &ACSKPlayerController::SelectTile);
	InputComponent->BindAction("AltSelect", IE_Pressed, this, &ACSKPlayerController::AltSelectTile);
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
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
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
	TestTile1 = GetTileUnderMouse();
	if (TestTile1 && TestTile2)
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (BoardManager)
		{
			BoardManager->FindPath(TestTile1, TestTile2, TestBoardPath);
		}
	}
	else
	{
		TestBoardPath.Reset();
	}
}

void ACSKPlayerController::AltSelectTile()
{
	TestTile2 = GetTileUnderMouse();
	if (TestTile1 && TestTile2)
	{
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (BoardManager)
		{
			BoardManager->FindPath(TestTile1, TestTile2, TestBoardPath);
		}
	}
	else
	{
		TestBoardPath.Reset();
	}
}

void ACSKPlayerController::SetCastleController(ACastleAIController* InController)
{
	if (HasAuthority())
	{
		if (InController && CastleController)
		{
			CastleController->Destroy();
		}

		CastleController = InController;
	}
}
