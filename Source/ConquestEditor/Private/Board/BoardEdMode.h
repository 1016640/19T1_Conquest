// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "EdMode.h"
#include "BoardEditorObject.h"

class ABoardManager;
class FUICommandList;

/** 
 * Editor for laying out the board in conquest 
 */
class FEdModeBoard : public FEdMode
{
public:

	const static FEditorModeID EM_Board;

public:

	FEdModeBoard();
	virtual ~FEdModeBoard() = default;

public:

	// Begin FEdMode Interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool UsesToolkits() const override;
	// End FEdMode Interface

	// Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End FGCObject Interface

private:

	/** Notify that the current level has changed */
	//void OnLevelChange();

	/** Notify that current map has changed */
	//void OnMapChange(uint32 Event);

public:

	/** Get the command list for UI prompts */
	TSharedRef<FUICommandList> GetUICommandList() const;

	FORCEINLINE UBoardEditorObject* GetBoardSettings() const { return BoardSettings; }
	FORCEINLINE ABoardManager* GetCachedBoardManager() const { return BoardManager.Get(); }

	/** If we are currently editing an existing board */
	FORCEINLINE bool IsEditingBoard() const { return BoardManager.IsValid(); }

private:

	/** Get the board manager of the current level */
	ABoardManager* GetLevelsBoardManager() const;

private:

	/** The board settings for either new or current board */
	UBoardEditorObject* BoardSettings;

	/** The board manager we are currently editing (if it exists) */
	TWeakObjectPtr<ABoardManager> BoardManager;

	/** Handles to bound callbacks */

};

