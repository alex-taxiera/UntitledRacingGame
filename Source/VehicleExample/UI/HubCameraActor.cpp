// Copyright Epic Games, Inc. All Rights Reserved.

#include "HubCameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AHubCameraActor::AHubCameraActor()
{
    PrimaryActorTick.bCanEverTick = false;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    SetRootComponent(CameraComponent);

    CameraComponent->FieldOfView     = 60.f;
    CameraComponent->bConstrainAspectRatio = false;
}

void AHubCameraActor::ActivateForPlayer()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC)
    {
        PC->SetViewTarget(this);
    }
}
