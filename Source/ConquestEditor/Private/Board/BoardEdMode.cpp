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

	// for now
	if (BoardManager.IsValid())
	{
		const TArray<ATile*> Tiles = BoardManager->GetHexGrid().GetAllTiles();
		for (ATile* Tile : Tiles)
		{
			if (Tile)
			{
				DrawHexagon(View, Viewport, PDI, Tile->GetActorLocation(), BoardManager->GetHexSize());
			}
		}
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

void FEdModeBoard::GenerateGrid()
{
	if (!IsEditingBoard())
	{
		FScopedTransaction Transaction(LOCTEXT("Undo", "Generate Grid"));

		// TODO: IsEditingBoard always return false, we can clear this check once it is put back to normal
		if (!BoardManager.IsValid())
		{
			UWorld* World = GetWorld();
			ABoardManager* NewBoardManager = World->SpawnActor<ABoardManager>(BoardSettings->New_BoardOrigin, FRotator::ZeroRotator);
			if (NewBoardManager)
			{
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

void FEdModeBoard::DrawHexagon(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI, const FVector& Position, float HexSize)
{
	check(Viewport);
	check(PDI);

	const float PerimeterSize = 5.f;
	const float CenterSize = HexSize * 0.25f;
	const FLinearColor PerimeterColor = FLinearColor::Green;
	const FLinearColor CenterColor = FLinearColor::Red;
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