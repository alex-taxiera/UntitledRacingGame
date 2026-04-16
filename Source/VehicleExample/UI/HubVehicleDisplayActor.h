// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HubVehicleDisplayActor.generated.h"

class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UVehicleDefinition;
class USpotLightComponent;

DECLARE_DELEGATE_OneParam(FOnRenderTargetReady, UTextureRenderTarget2D*);

/**
 * AHubVehicleDisplayActor
 *
 * A lightweight actor placed (or spawned) in the hub level whose sole
 * purpose is to render a vehicle mesh into a UTextureRenderTarget2D for
 * display in Slate UI panels (dealership and garage).
 *
 * Architecture:
 *   - USkeletalMeshComponent   — displays the vehicle mesh with no physics.
 *   - USceneCaptureComponent2D — captures the scene to a render target each frame.
 *   - Two USpotLightComponents — simple three-point-ish lighting for the display.
 *   - UTextureRenderTarget2D   — owned here; the hub game mode passes a pointer
 *                                to the Slate widgets so they can create SImage brushes.
 *
 * Usage:
 *   1. Spawn via AHubGameMode::BeginPlay (or place in the level).
 *   2. Call SetVehicle(Definition) to load and display a vehicle.
 *   3. Pass GetRenderTarget() to SHubRootWidget for the Slate brush.
 *
 * The actor lives entirely off to the side of the visible level geometry.
 * Its capture component uses a dedicated ShowOnlyList so only this actor
 * is captured, keeping the render target clean regardless of level content.
 */
UCLASS()
class VEHICLEEXAMPLE_API AHubVehicleDisplayActor : public AActor
{
    GENERATED_BODY()

public:

    AHubVehicleDisplayActor();

    virtual void BeginPlay() override;

    // -----------------------------------------------------------------------
    // Vehicle control
    // -----------------------------------------------------------------------

    /**
     * Loads the skeletal mesh from Definition->PreviewMesh and applies it.
     * Passing nullptr clears the mesh.
     * The render target immediately reflects the change on the next capture tick.
     */
    UFUNCTION(BlueprintCallable, Category = "HubDisplay")
    void SetVehicle(UVehicleDefinition* Definition);

    // -----------------------------------------------------------------------
    // Render target access
    // -----------------------------------------------------------------------

    /** Returns the render target this actor writes to. Never null after BeginPlay. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HubDisplay")
    UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

    /**
     * Fired at the end of BeginPlay once the render target is created and valid.
     * Bind before the actor's BeginPlay runs (i.e. bind immediately after spawning).
     */
    FOnRenderTargetReady OnRenderTargetReady;

    // -----------------------------------------------------------------------
    // Components (public for Blueprint subclassing if needed)
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USpotLightComponent> KeyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USpotLightComponent> FillLight;

private:

    /** Render target created at BeginPlay. Owned by this actor. */
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    /** Render target resolution. */
    static constexpr int32 RenderTargetSize = 1024;
};
