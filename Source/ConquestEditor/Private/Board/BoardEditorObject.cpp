// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEditorObject.h"
#include "BoardEdMode.h"
#include "Board/BoardManager.h"

UBoardEditorObject::UBoardEditorObject()
{
	BoardEdMode = nullptr;

	New_BoardRows = 10;
	New_BoardColumns = 10;
	New_BoardHexSize = 200.f;

	Edit_BoardRows = 10;
	Edit_BoardColumns = 10;
	Edit_BoardHexSize = 200.f;
}

#if WITH_EDITOR
void UBoardEditorObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (GET_MEMBER_NAME_CHECKED(UBoardEditorObject, BoardOrigin) == PropertyName)
	{
		if (BoardEdMode && BoardEdMode->IsEditingBoard())
		{
			ABoardManager* BoardManager = BoardEdMode->GetCachedBoardManager();
			BoardManager->SetActorLocation(BoardOrigin);
		}
	}
}
#endif

void UBoardEditorObject::SetEditorMode(FEdModeBoard* EdMode)
{
	BoardEdMode = EdMode;
}

void UBoardEditorObject::UpdateBoardEditProperties()
{
	if (BoardEdMode && BoardEdMode->IsEditingBoard())
	{
		ABoardManager* BoardManager = BoardEdMode->GetCachedBoardManager();
		check(BoardManager);

		Edit_BoardRows = 10;
		Edit_BoardColumns = 10;
		Edit_BoardHexSize = 200.f;
		BoardOrigin = BoardManager->GetActorLocation();
	}
}
