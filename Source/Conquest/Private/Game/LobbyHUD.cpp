// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyHUD.h"
#include "LobbyPlayerController.h"

#include "Widgets/LobbyMenuWidget.h"

ALobbyHUD::ALobbyHUD()
{
	bLoggedTemplateWarning = true;
}

void ALobbyHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UConquestFunctionLibrary::RemoveWidgetFromParent(LobbyWidgetInstance);
}

void ALobbyHUD::NotifyLobbyMembersRefreshed(ALobbyPlayerState* HostState, ALobbyPlayerState* GuestState)
{
	ULobbyMenuWidget* LobbyWidget = GetLobbyWidgetInstance();
	if (LobbyWidget)
	{
		LobbyWidget->OnLobbyMembersRefreshed(HostState, GuestState);
		UConquestFunctionLibrary::AddWidgetToViewport(LobbyWidget);
	}
}

void ALobbyHUD::NotifySelectedMapChanged(const FMapSelectionDetails& MapDetails)
{
	ULobbyMenuWidget* LobbyWidget = GetLobbyWidgetInstance();
	if (LobbyWidget)
	{
		LobbyWidget->OnSelectedMapChanged(MapDetails);
	}
}

ULobbyMenuWidget* ALobbyHUD::GetLobbyWidgetInstance()
{
	if (!LobbyWidgetInstance)
	{
		if (!LobbyWidgetTemplate)
		{
			// Avoid spam (as we will be calling this multiple times)
			if (!bLoggedTemplateWarning)
			{
				UE_LOG(LogConquest, Warning, TEXT("ALobbbyHUD::GetLobbyWidgetInstance: Menu template is null"));
				bLoggedTemplateWarning = true;
			}

			return nullptr;
		}

		LobbyWidgetInstance = CreateWidget<ULobbyMenuWidget, APlayerController>(PlayerOwner, LobbyWidgetTemplate);
		if (!LobbyWidgetInstance)
		{
			UE_LOG(LogConquest, Warning, TEXT("ALobbbyHUD::GetLobbyWidgetInstance: Failed to create Widget"));
		}
	}

	return LobbyWidgetInstance;
}
