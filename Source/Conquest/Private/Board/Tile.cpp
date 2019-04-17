// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"
#include "BoardManager.h"
#include "BoardPieceInterface.h"
#include "CSKPlayerController.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bNetLoadOnClient = true;
	bAlwaysRelevant = false;
	bOnlyRelevantToOwner = false;
	bReplicateMovement = false;

	USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(DummyRoot);
	DummyRoot->SetMobility(EComponentMobility::Static);

	// Fire tile type
	TileType = 1;
	bIsNullTile = false;
	GridHexIndex = FIntVector(-1);

	#if WITH_EDITORONLY_DATA
	bHighlightTile = false;
	#endif
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ATile::StartHoveringTile(ACSKPlayerController* Controller)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		// Null tiles do not recieve this event
		if (!bIsNullTile)
		{ 
			BP_OnHoverStart(Controller);
		}

		#if WITH_EDITORONLY_DATA
		bHighlightTile = true;
		#endif
	}
}

void ATile::EndHoveringTile(ACSKPlayerController* Controller)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		// Null tiles do not recieve this event
		if (!bIsNullTile)
		{
			BP_OnHoverEnd(Controller);
		}

		#if WITH_EDITORONLY_DATA
		bHighlightTile = false;
		#endif
	}
}

bool ATile::SetBoardPiece(AActor* BoardPiece)
{
	// Only set on the server
	if (!HasAuthority())
	{
		return false;
	}

	// Already occupied
	if (IsTileOccupied())
	{
		return false;
	}
	
	// Board piece is invalid
	if (!BoardPiece->Implements<UBoardPieceInterface>())
	{
		return false;
	}

	UE_LOG(LogConquest, Log, TEXT("Setting Board Piece %s for Tile %s (Hex Value %s)"),
		*BoardPiece->GetName(), *GetName(), *GridHexIndex.ToString());

	// Board piece is valid, have all clients update their occupant
	Multi_SetBoardPiece(BoardPiece);
	return true;
}

bool ATile::ClearBoardPiece()
{
	// Only clear on server
	if (!HasAuthority())
	{
		return false;
	}

	// No piece to clear
	if (!IsTileOccupied())
	{
		return false;
	}

	UE_LOG(LogConquest, Log, TEXT("Clearing Board Piece %s for Tile %s (Hex Value %s)"),
		*PieceOccupant.GetObject()->GetName(), *GetName(), *GridHexIndex.ToString());

	// We have a board piece to clear, have all clients update their occupant
	Multi_ClearBoardPiece();
	return true;
}

void ATile::Multi_SetBoardPiece_Implementation(AActor* BoardPiece)
{
	if (BoardPiece && ensure(BoardPiece->Implements<UBoardPieceInterface>()))
	{
		PieceOccupant = BoardPiece;
		PieceOccupant->PlacedOnTile(this);

		// Execute any events after set up is complete
		BP_OnBoardPieceSet(BoardPiece);
	}
	else
	{
		// If we are clients and the board piece is null
		if (!BoardPiece && !HasAuthority())
		{
			UE_LOG(LogConquest, Warning, TEXT("ATile::Multi_SetBoardPiece: Board Piece is null on the client. "
				"This board piece may not be replicating or has yet to replicate."))
		}
	}
}

void ATile::Multi_ClearBoardPiece_Implementation()
{
	if (PieceOccupant.GetInterface() != nullptr)
	{
		AActor* BoardPiece = CastChecked<AActor>(PieceOccupant.GetObject());
		
		// Execute any events before clearing (as at this point we are still technically occupied)
		BP_OnBoardPieceCleared();

		PieceOccupant->RemovedOffTile();
		PieceOccupant = nullptr;
	}
}

bool ATile::IsTileOccupied(bool bConsiderNull) const
{
	if (PieceOccupant.GetInterface() == nullptr)
	{
		return bConsiderNull && bIsNullTile;
	}

	return true;
}

bool ATile::DoesPlayerOccupyTile(const ACSKPlayerState* Player) const
{
	if (PieceOccupant.GetInterface() != nullptr)
	{
		return Player && PieceOccupant->GetBoardPieceOwnerPlayerState() == Player;
	}

	return false;
}

bool ATile::CanPlaceTowersOn() const
{
	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	return BoardManager ? BoardManager->CanPlaceTowerOnTile(this) : false;
}

AActor* ATile::GetBoardPiece() const
{
	if (IsTileOccupied())
	{
		return CastChecked<AActor>(PieceOccupant.GetObject());
	}

	return nullptr;
}

ACSKPlayerState* ATile::GetBoardPiecesOwner() const
{
	if (IsTileOccupied())
	{
		return PieceOccupant->GetBoardPieceOwnerPlayerState();
	}

	return nullptr;
}

UHealthComponent* ATile::GetBoardPieceHealthComponent() const
{
	if (IsTileOccupied())
	{
		return PieceOccupant->GetHealthComponent();
	}

	return nullptr;
}
