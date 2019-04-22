// Fill out your copyright notice in the Description page of Project Settings.

#include "Tile.h"
#include "BoardManager.h"
#include "BoardPieceInterface.h"
#include "CSKHUD.h"
#include "CSKPlayerController.h"
#include "CSKPlayerState.h"

#include "Components/StaticMeshComponent.h"

ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bNetLoadOnClient = true;
	bAlwaysRelevant = false;
	bOnlyRelevantToOwner = false;
	bReplicateMovement = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Static);

	// Fire tile type
	TileType = ECSKElementType::Fire;
	bIsNullTile = false;
	GridHexIndex = FIntVector(-1);
	SelectionState = ETileSelectionState::NotSelectable;

	#if WITH_EDITORONLY_DATA
	bHighlightTile = false;
	#endif
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

#if WITH_EDITOR
void ATile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// We will need to find the board manager manually if not in PIE
	ABoardManager* BoardManager = nullptr;
	
	UWorld* World = GetWorld();
	if (World && World->IsPlayInEditor())
	{
		BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(World, false);
	}
	else
	{
		BoardManager = UConquestFunctionLibrary::FindMatchBoardManager(this, false);
	}

	// We don't need to do anything if there is no board manager
	if (!BoardManager)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATile, TileType) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ATile, bIsNullTile))
	{
		BoardManager->SetTilesHighlightMaterial(this);
	}
}
#endif

void ATile::StartHoveringTile(ACSKPlayerController* Controller)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		// Null tiles do not recieve this event
		if (!bIsNullTile)
		{ 
			BP_OnHoverStart(Controller);
		}

		// Notify the board piece on this tile
		if (PieceOccupant.GetInterface() != nullptr)
		{
			PieceOccupant->OnHoverStart();
		}

		HoveringPlayer = Controller;

		RefreshHighlightMaterial();

		#if WITH_EDITORONLY_DATA
		bHighlightTile = true;
		#endif
	}
}

void ATile::EndHoveringTile(ACSKPlayerController* Controller)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		check(HoveringPlayer == Controller);

		// Null tiles do not recieve this event
		if (!bIsNullTile)
		{
			BP_OnHoverEnd(Controller);
		}

		// Notify the board piece on this tile
		if (PieceOccupant.GetInterface() != nullptr)
		{
			PieceOccupant->OnHoverFinish();
		}

		HoveringPlayer.Reset();

		RefreshHighlightMaterial();

		#if WITH_EDITORONLY_DATA
		bHighlightTile = false;
		#endif
	}
}

void ATile::SetSelectionState(ETileSelectionState State)
{
	if (SelectionState != State)
	{
		SelectionState = State;
		RefreshHighlightMaterial();
	}
}

void ATile::RefreshHoveringPlayersBoardPieceUI()
{
	if (HoveringPlayer.IsValid())
	{
		check(HoveringPlayer->IsLocalPlayerController());

		// This will collect recent BoardPieceUIData to refresh the display with
		ACSKHUD* CSKHUD = HoveringPlayer->GetCSKHUD();
		if (CSKHUD)
		{
			CSKHUD->OnTileHovered(this);
		}
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

		// We want to call this after placed on tile, as a board piece
		// can never be hovered when not on a tile to hover over
		if (IsHovered())
		{
			PieceOccupant->OnHoverStart();
		}

		// Execute any events after set up is complete
		BP_OnBoardPieceSet(BoardPiece);

		// These should always be executed last
		RefreshHighlightMaterial();
		RefreshHoveringPlayersBoardPieceUI();
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
		
		// Execute any events before clearing
		BP_OnBoardPieceCleared();

		// We want to call this before removed off tile, as a board
		// piece can never be unhovered when not on a tile to hover off
		if (IsHovered())
		{
			PieceOccupant->OnHoverFinish();
		}

		PieceOccupant->RemovedOffTile();
		PieceOccupant = nullptr;

		// These should always be executed last
		RefreshHighlightMaterial();
		RefreshHoveringPlayersBoardPieceUI();
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
	if (IsTileOccupied(false))
	{
		return CastChecked<AActor>(PieceOccupant.GetObject());
	}

	return nullptr;
}

ACSKPlayerState* ATile::GetBoardPiecesOwner() const
{
	if (IsTileOccupied(false))
	{
		return PieceOccupant->GetBoardPieceOwnerPlayerState();
	}

	return nullptr;
}

int32 ATile::GetBoardPiecesOwnerPlayerID() const
{
	ACSKPlayerState* OwnerState = GetBoardPiecesOwner();
	if (OwnerState)
	{
		return OwnerState->GetCSKPlayerID();
	}

	return -1;
}

UHealthComponent* ATile::GetBoardPieceHealthComponent() const
{
	if (IsTileOccupied(false))
	{
		return PieceOccupant->GetHealthComponent();
	}

	return nullptr;
}

FBoardPieceUIData ATile::GetBoardPieceUIData() const
{
	FBoardPieceUIData UIData;

	if (IsTileOccupied())
	{
		UIData.BoardPiece = CastChecked<AActor>(PieceOccupant.GetObject());
		UIData.Owner = PieceOccupant->GetBoardPieceOwnerPlayerState();

		// Allow individual board pieces to pass in additional data
		PieceOccupant->GetBoardPieceUIData(UIData);
	}

	return UIData;
}

void ATile::RefreshHighlightMaterial()
{
	ABoardManager* BoardManager = UConquestFunctionLibrary::GetMatchBoardManager(this);
	if (BoardManager)
	{
		BoardManager->SetTilesHighlightMaterial(this);
	}
}
