// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEditorObject.h"
#include "BoardEdMode.h"
#include "Board/BoardManager.h"
#include "Board/Tile.h"

UBoardEditorObject::UBoardEditorObject()
{
	BoardEdMode = nullptr;

	BoardRows = 10;
	BoardColumns = 10;
	BoardHexSize = 200.f;
	BoardOrigin = FVector::ZeroVector;
	BoardTileTemplate = ATile::StaticClass();

	LastBoardsTileType = nullptr;
	bWarnOfTileDifference = false;
}

#if WITH_EDITOR
void UBoardEditorObject::PreEditChange(UProperty* Property)
{
	Super::PreEditChange(Property);
}

void UBoardEditorObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	if (GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardOrigin) == PropertyName)
	{
		if (BoardEdMode && BoardEdMode->GetCachedBoardManager() != nullptr)//BoardEdMode->IsEditingBoard())
		{
			ABoardManager* BoardManager = BoardEdMode->GetCachedBoardManager();
			BoardManager->SetActorLocation(BoardOrigin);
		}
	}
	if (GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardTileTemplate) == PropertyName)
	{
		// Should always have a valid tile
		if (!BoardTileTemplate)
		{
			BoardTileTemplate = ATile::StaticClass();
		}

		// We should warn the user of potential data loss if changing tile types
		bWarnOfTileDifference = LastBoardsTileType != nullptr && BoardTileTemplate != LastBoardsTileType;
	}

	if (BoardEdMode)
	{
		BoardEdMode->RefreshEditorWidget();
	}
}
#endif

void UBoardEditorObject::SetEditorMode(FEdModeBoard* EdMode)
{
	BoardEdMode = EdMode;
}

void UBoardEditorObject::UpdateBoardEditProperties()
{
	if (BoardEdMode && BoardEdMode->GetCurrentEditingState() != EBoardEditingState::GenerateBoard)
	{
		ABoardManager* BoardManager = BoardEdMode->GetCachedBoardManager();
		check(BoardManager);

		const FIntPoint& BoardDimensions = BoardManager->GetGridDimensions();
		BoardRows = BoardDimensions.X;
		BoardColumns = BoardDimensions.Y;

		BoardHexSize = BoardManager->GetGridHexSize();
		BoardOrigin = BoardManager->GetActorLocation();
		BoardTileTemplate = BoardManager->GetGridTileTemplate();

		LastBoardsTileType = BoardTileTemplate;
		bWarnOfTileDifference = false;
	}
}

void UBoardEditorObject::NotifyEditingStart()
{
	UpdateBoardEditProperties();
}

void UBoardEditorObject::NotifyBoardGenerated()
{
	LastBoardsTileType = BoardTileTemplate;
	bWarnOfTileDifference = false;
}
