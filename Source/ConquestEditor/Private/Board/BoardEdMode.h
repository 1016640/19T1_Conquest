// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ConquestEditor.h"
#include "EdMode.h"
#include "BoardEditorObject.h"

class ABoardManager;
class FBoardToolkit;
class FUICommandList;

/** Tracks the state for when editing a board */
enum class EBoardEditingState : uint8
{
	GenerateBoard,
	EditBoard,
	TileSelected
};

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

	virtual EEditAction::Type GetActionEditDuplicate() override;
	virtual EEditAction::Type GetActionEditDelete() override;
	virtual EEditAction::Type GetActionEditCut() override;
	virtual EEditAction::Type GetActionEditCopy() override;
	virtual EEditAction::Type GetActionEditPaste() override;
	virtual bool ProcessEditDuplicate() override;
	virtual bool ProcessEditDelete() override;
	virtual bool ProcessEditCut() override;
	virtual bool ProcessEditCopy() override;
	virtual bool ProcessEditPaste() override;

	virtual bool AllowWidgetMove() override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;
	virtual FVector GetWidgetLocation() const override;
	virtual EAxisList::Type GetWidgetAxisToDraw(FWidget::EWidgetMode InWidgetMode) const override;

	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	// End FEdMode Interface

	// Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End FGCObject Interface

public:

	/** Generates a new board based off current settings */
	void GenerateBoard();

private:

	/** Notify that the current level has changed */
	//void OnLevelChange();

	/** Notify that current map has changed */
	//void OnMapChange(uint32 Event);

private:

	/** Draws a preview render of the grid currently in creation */
	void DrawPreviewBoard(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) const;

	/** Draws the render of the existing board manager */
	void DrawExistingBoard(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) const;

	/** Draws a hexagon of given size at given location */
	void DrawHexagon(FViewport* Viewport, FPrimitiveDrawInterface* PDI, const FVector& Origin, float HexSize,
		const FLinearColor& PerimeterColor, const FLinearColor& CenterColor = FLinearColor::Transparent) const;

public:

	/** Get the command list for UI prompts */
	TSharedRef<FUICommandList> GetUICommandList() const;

	FORCEINLINE UBoardEditorObject* GetBoardSettings() const { return BoardSettings; }
	FORCEINLINE ABoardManager* GetCachedBoardManager() const { return BoardManager.Get(); }

	/** If we are currently editing an existing board */
	FORCEINLINE bool IsEditingBoard() const { return false; }// BoardManager.IsValid(); }

public:

	/** Informs editor widget to refresh */
	void RefreshEditorWidget() const;

private:

	/** Get the board manager of the current level */
	ABoardManager* GetLevelsBoardManager() const;

	/** Get toolkit as a board editor toolkit */
	TSharedPtr<FBoardToolkit> GetBoardToolkit() const;

private:

	/** The board settings for either new or current board */
	UBoardEditorObject* BoardSettings;

	/** The board manager we are currently editing (if it exists) */
	TWeakObjectPtr<ABoardManager> BoardManager;

	/** Handles to bound callbacks */

};

