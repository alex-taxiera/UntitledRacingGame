// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubCameraActor.generated.h"

class UCameraComponent;

/**
 * AHubCameraActor
 *
 * A minimal camera actor spawned by AHubGameMode at BeginPlay.
 * AHubGameMode calls ActivateForPlayer() which sets this actor as the
 * player controller's view target, giving the hub level a proper camera.
 *
 * Default position: pulled back and slightly elevated, facing the origin
 * where the hub scene geometry sits.  Override SpawnLocation /
 * SpawnRotation on AHubGameMode to reposition without recompiling.
 */
UCLASS()
class VEHICLEEXAMPLE_API AHubCameraActor : public AActor
{
    GENERATED_BODY()

public:

    AHubCameraActor();

    /** Sets this actor as the view target for the first local player. */
    void ActivateForPlayer();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> CameraComponent;
};
