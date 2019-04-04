// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerController.h"
#include "CSKGameState.h"
#include "CSKLocalPlayer.h"
#include "CSKPawn.h"
#include "CSKPlayerState.h"
#include "BoardManager.h"
#include "Castle.h"
#include "CastleAIController.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

// TODO: Remove
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

ACSKPlayerController::ACSKPlayerController()
{
	bShowMouseCursor = true;
}

void ACSKPlayerController::ClientSetHUD_Implementation(TSubclassOf<AHUD> NewHUDClass)
{
	Super::ClientSetHUD_Implementation(NewHUDClass);

	CachedCSKHUD = Cast<ACSKHUD>(MyHUD);
}

void ACSKPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocalPlayerController())
	{
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

		#if WITH_EDITORONLY_DATA
		// Help with debugging while in editor. This change is local to client
		if (HoveredTile)
		{
			HoveredTile->bHighlightTile = false;
		}
		#endif
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

void ACSKPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACSKPlayerController, CastlePawn, COND_AutonomousOnly);
}

ACSKPawn* ACSKPlayerController::GetCSKPawn() const
{
	return Cast<ACSKPawn>(GetPawn());
}

ACSKPlayerState* ACSKPlayerController::GetCSKPlayerState() const
{
	return GetPlayerState<ACSKPlayerState>();
}

ACSKHUD* ACSKPlayerController::GetCSKHUD() const
{
	return CachedCSKHUD;
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
	if (bIsActionPhase)
	{

	}
}

void ACSKPlayerController::AltSelectTile()
{

}

void ACSKPlayerController::SetCastleController(ACastleAIController* InController)
{
	if (HasAuthority())
	{
		if (InController && CastleController)
		{
			CastleController->Destroy();
		}

		// Our link to our castle
		CastleController = InController;
		CastlePawn = InController->GetCastle();

		// Link back to us
		CastleController->PlayerOwner = this;
	}
}

void ACSKPlayerController::OnTransitionToBoard(int32 PlayerID)
{
	if (ensure(HasAuthority()))
	{
		// Occupy the space we are on
		ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
		if (BoardManager)
		{
			ATile* PortalTile = BoardManager->GetPlayerPortalTile(PlayerID);
			check(PortalTile);

			if (!BoardManager->PlaceBoardPieceOnTile(CastlePawn, PortalTile))
			{
				UE_LOG(LogConquest, Warning, TEXT("Unable to set player %i castle on portal tile as it's already occupied!"), PlayerID);
			}
		}

		// Have client handle any local transition requirements
		Client_OnTransitionToBoard();
	}
}

void ACSKPlayerController::StartActionPhase()
{
	if (HasAuthority())
	{
		bIsActionPhase = true;
		
		RemainingActions = ECSKActionPhaseMode::All;
		SetActionMode(ECSKActionPhaseMode::MoveCastle);		
	}
}

bool ACSKPlayerController::CanRequestMoveAction() const
{
	if (bIsActionPhase && EnumHasAnyFlags(SelectedAction, ECSKActionPhaseMode::MoveCastle))
	{
		return true;
	}

	return false;
}

void ACSKPlayerController::Client_OnTransitionToBoard_Implementation()
{
	// Move the camera over to our castle (where our portal should be)
	ACSKPawn* Pawn = GetCSKPawn();
	if (Pawn)
	{
		if (CastlePawn)
		{
			Pawn->TravelToLocation(CastlePawn->GetActorLocation(), false);
		}
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("Client was unable to travel to castle as pawn was null! (Probaly not replicated yet)"));
	}
}

bool ACSKPlayerController::ServerMoveCastleTo_Validate(ATile* Tile)
{
	return true;
}

void ACSKPlayerController::ServerMoveCastleTo_Implementation(ATile* Tile)
{
	
}