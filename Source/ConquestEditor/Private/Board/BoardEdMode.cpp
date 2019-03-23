// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardEdMode.h"

const FEditorModeID FEdModeBoard::EM_BoardMode(TEXT("EM_BoardEdMode"));

void FEdModeBoard::Enter()
{
	FEdMode::Enter();

	UE_LOG(LogConquestEditor, Log, TEXT("Entered Conquest Board Editor mode"));
}

void FEdModeBoard::Exit()
{
	FEdMode::Exit();

	UE_LOG(LogConquestEditor, Log, TEXT("Exited Conquest Board Editor mode"));
}

void FEdModeBoard::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);
}
