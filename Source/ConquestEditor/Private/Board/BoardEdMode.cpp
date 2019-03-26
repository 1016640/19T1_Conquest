// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEdMode.h"
#include "BoardToolkit.h"
#include "Board/BoardManager.h"
#include "Board/Tile.h"

#include "EditorModeManager.h"
#include "EditorSupportDelegates.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"

#include "ScopedTransaction.h"
#include "ToolkitManager.h"
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

void FEdModeBoard::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(BoardSettings);
}

void FEdModeBoard::GenerateBoard()
{
	if (!IsEditingBoard())
	{
		FScopedTransaction Transaction(LOCTEXT("Undo", "Generate Grid"));

		// TODO: IsEditingBoard always return false, we can clear this check once it is put back to normal
		if (!BoardManager.IsValid())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = TEXT("BoardManager");
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			UWorld* World = GetWorld();
			ABoardManager* NewBoardManager = World->SpawnActor<ABoardManager>(BoardSettings->New_BoardOrigin, FRotator::ZeroRotator, SpawnParams);
			if (NewBoardManager)
			{
				NewBoardManager->SetActorLabel("BoardManager");
				BoardManager = NewBoardManager;
			}
			else
			{
				UE_LOG(LogConquestEditor, Warning, TEXT("Unable to spawn new board manager"));
			}	
		}

		if (BoardManager.IsValid())
		{
			FBoardInitData InitData(
				FIntPoint(BoardSettings->New_BoardRows, BoardSettings->New_BoardColumns),
				BoardSettings->New_BoardHexSize,
				BoardSettings->New_BoardOrigin,
				FRotator::ZeroRotator);

			BoardManager->InitBoard(InitData);

			// Notify the toolkit to update anything based around edit state
			GetBoardToolkit()->NotifyEditingStateChanged();
		}
		else
		{
			UE_LOG(LogConquestEditor, Warning, TEXT("Unable to generate grid as board manager was null"));
		}
	}
}

void FEdModeBoard::DrawPreviewBoard(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) const
{
	using FHex = FHexGrid::FHex;

	const int32 Rows = BoardSettings->New_BoardRows;
	const int32 Columns = BoardSettings->New_BoardColumns;
	const float HexSize = BoardSettings->New_BoardHexSize;
	const FVector SizeVec = FVector(HexSize);
	const FVector& Origin = BoardSettings->New_BoardOrigin;

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
			DrawHexagon(Viewport, PDI, Tile->GetActorLocation(), BoardManager->GetHexSize(), PerimeterColor);
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

#undef LOCTEXT_NAMESPACE