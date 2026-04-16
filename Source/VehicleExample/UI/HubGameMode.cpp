// Copyright Epic Games, Inc. All Rights Reserved.

#include "HubGameMode.h"
#include "HubVehicleDisplayActor.h"
#include "HubCameraActor.h"
#include "SHubRootWidget.h"
#include "RacingGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

const FVector AHubGameMode::DisplayActorLocation = FVector(0.f, 5000.f, 0.f);

AHubGameMode::AHubGameMode()
{
}

void AHubGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("=== HubGameMode::BeginPlay ==="));

    // Spawn the display actor off to the side, out of the player camera's view
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = TEXT("HubVehicleDisplayActor");
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    DisplayActor = GetWorld()->SpawnActor<AHubVehicleDisplayActor>(
        AHubVehicleDisplayActor::StaticClass(),
        DisplayActorLocation,
        FRotator::ZeroRotator,
        SpawnParams);

    // Spawn the hub camera and set it as the view target
    FActorSpawnParameters CamParams;
    CamParams.Name = TEXT("HubCameraActor");
    CamParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    HubCamera = GetWorld()->SpawnActor<AHubCameraActor>(
        AHubCameraActor::StaticClass(),
        CameraSpawnLocation,
        CameraSpawnRotation,
        CamParams);

    if (HubCamera)
    {
        HubCamera->ActivateForPlayer();
    }

    // Build and add the Slate widget
    // The render target isn't ready yet (BeginPlay runs after SpawnActor returns),
    // so we bind to OnRenderTargetReady and update the widget brush then.
    URacingGameInstance* GI = URacingGameInstance::Get(this);

    HubWidget = SNew(SHubRootWidget)
        .GameInstance(GI)
        .DisplayActor(DisplayActor);

    // Bind RT-ready callback — fires at end of DisplayActor::BeginPlay
    if (DisplayActor)
    {
        DisplayActor->OnRenderTargetReady.BindLambda(
            [this](UTextureRenderTarget2D* RT)
            {
                if (HubWidget.IsValid())
                {
                    HubWidget->NotifyRenderTargetReady(RT);
                }
            });
    }

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->AddViewportWidgetContent(
            HubWidget.ToSharedRef(),
            /* ZOrder */ 10);
    }

    // Show cursor and restrict input to UI
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
    }
}

void AHubGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HubWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(HubWidget.ToSharedRef());
    }
    HubWidget.Reset();

    Super::EndPlay(EndPlayReason);
}
