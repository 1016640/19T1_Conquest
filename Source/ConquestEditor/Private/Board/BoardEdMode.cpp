// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEdMode.h"
#include "BoardToolkit.h"
#include "Board/BoardManager.h"

#include "EditorModeManager.h"
#include "EditorSupportDelegates.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"

#include "ScopedTransaction.h"
#include "ToolkitManager.h"
#include "Engine/Selection.h"
#include "Engine/World.h"

const FEditorModeID FEdModeBoard::EM_Board(TEXT("EM_BoardEdMode"));

#define LOCTEXT_NAMESPACE "EdModeBoard"

FEdModeBoard::FEdModeBoard()
{
	BoardSettings = NewObject<UBoardEditorObject>(GetTransientPackage(), TEXT("BoardSettings"), RF_Transactional);
	BoardSettings->SetEditorMode(this);
}

void FEdModeBoard::Enter()
{
	FEdMode::Enter();

	GEditor->SelectNone(false, true);

	// Have board selected if we will be editing it
	BoardManager = GetLevelsBoardManager();
	if (BoardManager.IsValid())
	{
		GEditor->SelectActor(BoardManager.Get(), true, false);
	}

	// Initialize toolkit
	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FBoardToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	// Notify settings to match potential existing board
	BoardSettings->NotifyEditingStart();
}

void FEdModeBoard::Exit()
{
	// Shutdown our toolkit
	if (Toolkit.IsValid())
	{
		FToolkitManager& ToolkitManager = FToolkitManager::Get();
		ToolkitManager.CloseToolkit(Toolkit.ToSharedRef());

		Toolkit.Reset();
	}

	// Will clear screen from our previous render
	GEditor->RedrawLevelEditingViewports();

	FEdMode::Exit();
}

void FEdModeBoard::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	if (BoardManager.IsValid())
	{
		DrawExistingBoard(View, Viewport, PDI);
	}
	else
	{
		DrawPreviewBoard(View, Viewport, PDI);
	}
}

bool FEdModeBoard::UsesToolkits() const
{
	return true;
}

EEditAction::Type FEdModeBoard::GetActionEditDuplicate()
{
	return EEditAction::Skip;
}

EEditAction::Type FEdModeBoard::GetActionEditDelete()
{
	if (BoardManager.IsValid() && BoardManager->IsSelected())
	{
		return EEditAction::Process;
	}

	return EEditAction::Skip;
}

EEditAction::Type FEdModeBoard::GetActionEditCut()
{
	return EEditAction::Halt;
}

EEditAction::Type FEdModeBoard::GetActionEditCopy()
{
	return EEditAction::Process;
}

EEditAction::Type FEdModeBoard::GetActionEditPaste()
{
	return EEditAction::Process;
}

bool FEdModeBoard::ProcessEditDuplicate()
{
	return true;
}

bool FEdModeBoard::ProcessEditDelete()
{
	if (BoardManager.IsValid() && BoardManager->IsSelected())
	{
		BoardManager->DestroyBoard();
		BoardManager.Reset();
	}

	return true;
}

bool FEdModeBoard::ProcessEditCut()
{
	return true;
}

bool FEdModeBoard::ProcessEditCopy()
{
	return true;
}

bool FEdModeBoard::ProcessEditPaste()
{
	return true;
}

bool FEdModeBoard::AllowWidgetMove()
{
	return true;
}

bool FEdModeBoard::ShouldDrawWidget() const
{
	return UsesTransformWidget();
}

bool FEdModeBoard::UsesTransformWidget() const
{
	return true;
}

FVector FEdModeBoard::GetWidgetLocation() const
{
	if (BoardManager.IsValid())
	{
		return BoardManager->GetActorLocation();
	}
	else
	{
		return BoardSettings->BoardOrigin;
	}
}

EAxisList::Type FEdModeBoard::GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const
{
	switch (InWidgetMode)
	{
		case FWidget::WM_Translate:
			return EAxisList::XYZ;
		case FWidget::WM_Rotate:
			return EAxisList::None;
		case FWidget::WM_Scale:
			return BoardManager.IsValid() ? EAxisList::XYZ : EAxisList::X;
		default:
			return EAxisList::None;
	}
}

bool FEdModeBoard::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	if (!BoardManager.IsValid())
	{
		if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
		{
			BoardSettings->Modify();
			BoardSettings->BoardOrigin += InDrag;
			BoardSettings->BoardHexSize = FMath::Max(10.f, BoardSettings->BoardHexSize + InScale.X * 10.f);

			return true;
		}
	}
	else
	{
		if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
		{
			FVector NewLocation = BoardManager->GetActorLocation() + InDrag;

			BoardManager->Modify();
			BoardManager->SetActorLocation(NewLocation);
			BoardManager->SetActorScale3D(BoardManager->GetActorScale() + InScale);

			BoardSettings->Modify();
			BoardSettings->BoardOrigin = NewLocation;

			return true;
		}
	}

	return false;
}

void FEdModeBoard::ActorSelectionChangeNotify()
{
	RefreshEditorWidget();
}

void FEdModeBoard::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(BoardSettings);
}

void FEdModeBoard::GenerateBoard()
{
	FScopedTransaction Transaction(LOCTEXT("Undo", "Generate Grid"));

	// Spawn in new board if forced
	if (!BoardManager.IsValid())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = TEXT("BoardManager");
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		UWorld* World = GetWorld();
		ABoardManager* NewBoardManager = World->SpawnActor<ABoardManager>(BoardSettings->BoardOrigin, FRotator::ZeroRotator, SpawnParams);
		if (NewBoardManager)
		{
			NewBoardManager->SetActorLabel("BoardManager");
			BoardManager = NewBoardManager;
		}
		else
		{
			UE_LOG(LogConquestEditor, Warning, TEXT("Unable to spawn new board manager"));
			Transaction.Cancel();
		}
	}

	if (BoardManager.IsValid())
	{
		FBoardInitData InitData(
			FIntPoint(BoardSettings->BoardRows, BoardSettings->BoardColumns),
			BoardSettings->BoardHexSize,
			BoardSettings->BoardOrigin,
			FRotator::ZeroRotator,
			BoardSettings->BoardTileTemplate);

		BoardManager->InitBoard(InitData);

		// Post generation notifies // TODO: Use a delegate?
		BoardSettings->NotifyBoardGenerated();
		GetBoardToolkit()->NotifyEditingStateChanged(); // TODO: rename

		// We might have a tile that did exist but is now dead selected, so default to selecting the board
		GEditor->SelectActor(BoardManager.Get(), true, true);
	}
	else
	{
		UE_LOG(LogConquestEditor, Warning, TEXT("Unable to generate grid as board manager was null"));
		Transaction.Cancel();
	}
}

void FEdModeBoard::DrawPreviewBoard(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) const
{
	using FHex = FHexGrid::FHex;

	const int32 Rows = BoardSettings->BoardRows;
	const int32 Columns = BoardSettings->BoardColumns;
	const float HexSize = BoardSettings->BoardHexSize;
	const FVector SizeVec = FVector(HexSize);
	const FVector& Origin = BoardSettings->BoardOrigin;

	const FLinearColor PreviewPerimeterColor = FLinearColor::Green;
	const FLinearColor PreviewCenterColor = FLinearColor::Red;

	for (int32 c = 0; c < Columns; ++c)
	{
		int32 COffset = FMath::FloorToInt(c / 2);
		for (int32 r = -COffset; r < Rows - COffset; ++r)
		{
			// Need hex value to determine location
			FHex Hex = FHexGrid::ConvertIndicesToHex(r, c);

			FVector TileLocation = FHexGrid::ConvertHexToWorld(Hex, Origin, SizeVec);
			DrawHexagon(Viewport, PDI, TileLocation, HexSize, PreviewPerimeterColor, PreviewCenterColor);
		}
	}
}

void FEdModeBoard::DrawExistingBoard(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) const
{
	check(BoardManager.IsValid());

	const FLinearColor PerimeterColor = FLinearColor::Yellow;

	// Simply draw every tile
	const TArray<ATile*> Tiles = BoardManager->GetHexGrid().GetAllTiles();
	for (ATile* Tile : Tiles)
	{
		if (Tile)
		{
			DrawHexagon(Viewport, PDI, Tile->GetActorLocation(), BoardManager->GetGridHexSize(), PerimeterColor);
		}
	}
}

void FEdModeBoard::DrawHexagon(FViewport* Viewport, FPrimitiveDrawInterface* PDI, const FVector& Position, float HexSize, 
	const FLinearColor& PerimeterColor, const FLinearColor& CenterColor) const
{
	check(Viewport);
	check(PDI);

	const float PerimeterSize = 5.f;
	const float CenterSize = HexSize * 0.25f;
	const ELevelViewportType ViewportType = static_cast<FEditorViewportClient*>(Viewport->GetClient())->ViewportType;

	// Get the location of a hexagons vertex based on index
	static auto GetHexVertex = [](const FVector& Position, float Size, int32 Index)->FVector
	{
		float Angle = 60.f * static_cast<float>(Index) - 30.f;
		float Radians = FMath::DegreesToRadians(Angle);

		return FVector(
			Position.X + Size * FMath::Cos(Radians),
			Position.Y + Size * FMath::Sin(Radians),
			Position.Z);
	};

	// Connect lines to form a hexagon
	FVector CurrentVertex = GetHexVertex(Position, HexSize, 0);
	for (int32 i = 0; i <= 5; ++i)
	{
		FVector NextVertex = GetHexVertex(Position, HexSize, (i + 1) % 6);

		if (ViewportType == LVT_Perspective)
		{
			PDI->DrawLine(CurrentVertex, NextVertex, PerimeterColor, SDPG_World, PerimeterSize);
		}

		CurrentVertex = NextVertex;
	}

	// Draw final point to show tiles middle location
	if (ViewportType == LVT_Perspective)
	{
		PDI->DrawLine(Position, Position, CenterColor, SDPG_World, CenterSize);
	}
}

TSharedRef<FUICommandList> FEdModeBoard::GetUICommandList() const
{
	check(Toolkit.IsValid());
	return Toolkit->GetToolkitCommands();
}

EBoardEditingState FEdModeBoard::GetCurrentEditingState() const
{
	if (!BoardManager.IsValid())
	{
		return EBoardEditingState::GenerateBoard;
	}

	// We might have a tile selected
	EBoardEditingState CurrentState = EBoardEditingState::EditBoard;
	if (GetNumSelectedTiles() >= 1)
	{
		CurrentState = EBoardEditingState::TileSelected;
	}

	return CurrentState;
}

void FEdModeBoard::RefreshEditorWidget() const
{
	GetBoardToolkit()->RefreshEditorWidget();
}

ABoardManager* FEdModeBoard::GetLevelsBoardManager() const
{
	UWorld* World = GetWorld();
	for (TActorIterator<ABoardManager> It(GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

TSharedPtr<FBoardToolkit> FEdModeBoard::GetBoardToolkit() const
{
	return StaticCastSharedPtr<FBoardToolkit>(Toolkit);
}

TArray<ATile*> FEdModeBoard::GetAllSelectedTiles() const
{
	TArray<ATile*> SelectedTiles;
	for (FSelectionIterator It = GEditor->GetSelectedActorIterator(); It; ++It)
	{
		ATile* Tile = Cast<ATile>(*It);
		if (Tile)
		{
			SelectedTiles.Add(Tile);
		}
	}

	return SelectedTiles;
}

int32 FEdModeBoard::GetNumSelectedTiles() const
{
	return GetAllSelectedTiles().Num();
}

#undef LOCTEXT_NAMESPACE