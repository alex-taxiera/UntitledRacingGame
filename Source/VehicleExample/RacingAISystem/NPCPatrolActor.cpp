// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCPatrolActor.h"
#include "RacingAIController.h"
#include "RacingSplineComponent.h"
#include "NPCRacerData.h"
#include "VehicleExamplePawn.h"
#include "Engine/World.h"
#include "RacingVehicleSystem/OwnedVehicle.h"

ANPCPatrolActor::ANPCPatrolActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Plain scene component as root — the challenge trigger is now distance-based in Tick.
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void ANPCPatrolActor::BeginPlay()
{
    Super::BeginPlay();
}

void ANPCPatrolActor::EndPlay(const EEndPlayReason::Type Reason)
{
    if (NPCPawn)
    {
        NPCPawn->Destroy();
        NPCPawn = nullptr;
    }
    Super::EndPlay(Reason);
}

// ---------------------------------------------------------------------------
// Tick — distance-based challenge trigger
// ---------------------------------------------------------------------------

void ANPCPatrolActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bInBattle || !PlayerPawn || !NPCPawn) { return; }

    const float DistSq    = FVector::DistSquared(PlayerPawn->GetActorLocation(),
                                                  NPCPawn->GetActorLocation());
    const float ThreshSq  = ChallengeRadius * ChallengeRadius;

    if (!bPlayerInRange && DistSq <= ThreshSq)
    {
        bPlayerInRange = true;
        OnChallenged.ExecuteIfBound(this);
    }
    else if (bPlayerInRange && DistSq > ThreshSq)
    {
        bPlayerInRange = false;
        OnChallengeLeft.ExecuteIfBound(this);
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void ANPCPatrolActor::Initialise(
    UNPCRacerData*         InRacerData,
    URacingSplineComponent* InPatrolSpline,
    AVehicleExamplePawn*    InPlayerPawn)
{
    RacerData    = InRacerData;
    PatrolSpline = InPatrolSpline;
    PlayerPawn   = InPlayerPawn;
    bInitialised = true;

    SpawnNPCPawn();
}

void ANPCPatrolActor::SpawnNPCPawn()
{
    if (!RacerData) { return; }

    // Resolve the pawn class from the vehicle definition
    TSoftClassPtr<APawn> SoftClass;
    if (RacerData->VehicleConfig.VehicleDefinition.IsValid())
    {
        UVehicleDefinition* Def = RacerData->VehicleConfig.VehicleDefinition.Get();
        if (!Def) { Def = RacerData->VehicleConfig.VehicleDefinition.LoadSynchronous(); }
        if (Def) { SoftClass = Def->PawnClass; }
    }

    UClass* PawnClass = SoftClass.IsValid()
        ? SoftClass.LoadSynchronous()
        : nullptr;

    if (!PawnClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ANPCPatrolActor: No PawnClass set on vehicle definition for NPC '%s'. Skipping spawn."),
            RacerData ? *RacerData->RacerName.ToString() : TEXT("Unknown"));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    NPCPawn = GetWorld()->SpawnActor<AVehicleExamplePawn>(
        PawnClass, GetActorTransform(), Params);

    if (!NPCPawn) { return; }

    // Spawn and possess with a RacingAIController
    AIController = GetWorld()->SpawnActor<ARacingAIController>(
        ARacingAIController::StaticClass());

    if (!AIController) { return; }

    // Configure the controller BEFORE Possess so OwnPawn is valid
    // when StartIdle() is called.
    AIController->SetRacerData(RacerData);
    AIController->SetPatrolSpline(PatrolSpline);
    // Only override the racing spline if we actually have a patrol spline.
    // If null, BeginPlay's auto-find result is preserved as fallback.
    if (PatrolSpline)
    {
        AIController->SetRacingSpline(PatrolSpline);
    }
    if (PlayerPawn)
    {
        AIController->SetPlayerPawn(PlayerPawn);
    }

    AIController->Possess(NPCPawn);
    AIController->StartIdle();

    // Apply data-asset vehicle stats (mass, RPM, torque, gear ratios, grip)
    // so NPC physics match their configured definition.
    UOwnedVehicle* NPCOwnedVehicle = RacerData->BuildOwnedVehicle(this);
    if (NPCOwnedVehicle)
    {
        NPCPawn->ApplyVehicleStats(NPCOwnedVehicle->ComputeEffectiveStats());
    }
}

// ---------------------------------------------------------------------------
// Race lifecycle
// ---------------------------------------------------------------------------

void ANPCPatrolActor::StartBattle()
{
    if (!AIController || bInBattle) { return; }
    bInBattle      = true;
    bPlayerInRange = false;  // reset so re-approach after battle works cleanly
    AIController->StartRace();
}

void ANPCPatrolActor::EndBattle()
{
    if (!AIController) { return; }
    bInBattle      = false;
    bPlayerInRange = false;  // prevent spurious re-challenge if player is still nearby
    AIController->EndRace();
    AIController->StartIdle();
}

void ANPCPatrolActor::LogAIDiagnostics() const
{
    if (!AIController)
    {
        UE_LOG(LogTemp, Error, TEXT("  NPC DIAG: AIController is NULL"));
        return;
    }
    UE_LOG(LogTemp, Warning,
        TEXT("  NPC DIAG: AIController='%s' | OwnPawn=%s | RacerData=%s | IdleActive=%d | RaceActive=%d | PatrolSpline=%s | RacingSpline=%s"),
        *AIController->GetClass()->GetName(),
        AIController->GetOwnPawn()      ? TEXT("OK") : TEXT("NULL"),
        AIController->GetRacerData()    ? TEXT("OK") : TEXT("NULL"),
        AIController->IsIdleActive(),
        AIController->IsRaceActive(),
        AIController->GetPatrolSpline() ? TEXT("OK") : TEXT("NULL"),
        AIController->GetRacingSpline() ? TEXT("OK") : TEXT("NULL"));
}

// ---------------------------------------------------------------------------
// Diagnostics
