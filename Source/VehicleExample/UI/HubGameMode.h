// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HubGameMode.generated.h"

class AHubVehicleDisplayActor;
class SHubRootWidget;

/**
 * AHubGameMode
 *
 * Game mode for the hub level (dealership + garage + menu).
 * Owns the AHubVehicleDisplayActor, the shared render target, and the
 * SHubRootWidget Slate tree.
 *
 * To use:
 *   1. Create a hub level in the editor.
 *   2. Set its World Settings ? Game Mode Override to AHubGameMode
 *      (or a Blueprint subclass).
 *   3. Ensure URacingGameInstance is the Game Instance Class in Project Settings.
 *   4. Set RacingGameInstance::GameLevelName to match this hub level's name.
 *
 * The display actor is spawned at DisplayActorLocation (5000 cm off to the
 * side of world origin) so it is never visible to the player camera.
 * Its SceneCaptureComponent2D uses a show-only list so surrounding level
 * geometry is invisible in the rendered UI image.
 */
UCLASS()
class VEHICLEEXAMPLE_API AHubGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    AHubGameMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

    /** Spawned at BeginPlay; owns the mesh + render target. */
    UPROPERTY()
    TObjectPtr<AHubVehicleDisplayActor> DisplayActor;

    /** The root Slate widget added to the viewport. */
    TSharedPtr<SHubRootWidget> HubWidget;

    /** World-space location where the display actor is placed out of view. */
    static const FVector DisplayActorLocation;
};
