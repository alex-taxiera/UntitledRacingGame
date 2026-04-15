// Copyright Epic Games, Inc. All Rights Reserved.

#include "TitleScreenGameMode.h"
#include "STitleScreenWidget.h"
#include "RacingGameInstance.h"
#include "Engine/GameViewportClient.h"

ATitleScreenGameMode::ATitleScreenGameMode()
{
}

void ATitleScreenGameMode::BeginPlay()
{
    Super::BeginPlay();

    URacingGameInstance* GI = URacingGameInstance::Get(this);

    TitleWidget = SNew(STitleScreenWidget)
        .GameInstance(GI);

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->AddViewportWidgetContent(
            TitleWidget.ToSharedRef(),
            /* ZOrder */ 10);
    }

    // Hide the mouse cursor default and show it for the menu
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor          = true;
        PC->SetInputMode(FInputModeUIOnly());
    }
}

void ATitleScreenGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (TitleWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(TitleWidget.ToSharedRef());
    }

    TitleWidget.Reset();

    Super::EndPlay(EndPlayReason);
}
