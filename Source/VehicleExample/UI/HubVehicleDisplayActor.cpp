// Copyright Epic Games, Inc. All Rights Reserved.

#include "HubVehicleDisplayActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RacingVehicleSystem/VehicleDefinition.h"

AHubVehicleDisplayActor::AHubVehicleDisplayActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Root — simple scene component so we can offset from world origin
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // Vehicle mesh
    MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(Root);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetCastShadow(false);

    // Key light — front-left
    KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
    KeyLight->SetupAttachment(Root);
    KeyLight->SetRelativeLocation(FVector(-300.f, -200.f, 300.f));
    KeyLight->SetRelativeRotation(FRotator(-45.f, 30.f, 0.f));
    KeyLight->SetIntensity(8000.f);
    KeyLight->SetAttenuationRadius(1500.f);
    KeyLight->SetCastShadows(false);

    // Fill light — front-right, softer
    FillLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FillLight"));
    FillLight->SetupAttachment(Root);
    FillLight->SetRelativeLocation(FVector(-300.f, 200.f, 200.f));
    FillLight->SetRelativeRotation(FRotator(-30.f, -30.f, 0.f));
    FillLight->SetIntensity(3000.f);
    FillLight->SetAttenuationRadius(1500.f);
    FillLight->SetCastShadows(false);

    // Scene capture
    CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
    CaptureComponent->SetupAttachment(Root);
    CaptureComponent->SetRelativeLocation(FVector(-500.f, 0.f, 100.f));
    CaptureComponent->SetRelativeRotation(FRotator(-5.f, 0.f, 0.f));
    CaptureComponent->FOVAngle = 45.f;
    CaptureComponent->bCaptureEveryFrame = true;
    CaptureComponent->bCaptureOnMovement = false;
    CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    CaptureComponent->ShowFlags.SetLighting(true);
    CaptureComponent->ShowFlags.SetPostProcessing(false);
}

void AHubVehicleDisplayActor::BeginPlay()
{
    Super::BeginPlay();

    // Create the render target
    RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("HubDisplayRT"));
    RenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
    RenderTarget->UpdateResourceImmediate(true);

    CaptureComponent->TextureTarget = RenderTarget;

    // Only capture this actor's mesh component
    CaptureComponent->ShowOnlyComponents.Add(MeshComponent);

    // Notify listeners that the render target is ready
    OnRenderTargetReady.ExecuteIfBound(RenderTarget);
}

void AHubVehicleDisplayActor::SetVehicle(UVehicleDefinition* Definition)
{
    if (!Definition)
    {
        MeshComponent->SetSkeletalMesh(nullptr);
        return;
    }

    USkeletalMesh* Mesh = Definition->PreviewMesh.LoadSynchronous();
    MeshComponent->SetSkeletalMesh(Mesh);

    // Re-register in the show-only list in case it was cleared
    if (!CaptureComponent->ShowOnlyComponents.Contains(MeshComponent))
    {
        CaptureComponent->ShowOnlyComponents.Add(MeshComponent);
    }
}
