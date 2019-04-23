// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuPlayerController.h"
#include "MainMenuGameMode.h"

#include "UserWidget.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
	bShowMouseCursor = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetInputMode(FInputModeGameAndUI());
	SetActiveWidget(DefaultActiveWidget);
}

void AMainMenuPlayerController::SetActiveWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	UConquestFunctionLibrary::RemoveWidgetFromParent(ActiveWidget);
	ActiveWidget = nullptr;

	if (WidgetClass)
	{
		ActiveWidget = CreateWidget<UUserWidget, APlayerController>(this, WidgetClass);
		UConquestFunctionLibrary::AddWidgetToViewport(ActiveWidget);
	}
}