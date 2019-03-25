// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEdMode.h"
#include "BoardToolkit.h"
#include "Board/BoardManager.h"

#include "EditorModeManager.h"
#include "EditorSupportDelegates.h"
#include "EngineUtils.h"
#include "ToolkitManager.h"
#include "Engine/World.h"

const FEditorModeID FEdModeBoard::EM_Board(TEXT("EM_BoardEdMode"));

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
