// Copyright Epic Games, Inc. All Rights Reserved.

#include "CourseGameMode.h"
#include "CourseSplineActor.h"
#include "CourseNPCSpawnManager.h"
#include "NPCPatrolActor.h"
#include "RacingSplineComponent.h"
#include "NPCRacerData.h"
#include "VehicleExamplePawn.h"
#include "RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleInventory.h"
#include "RacingVehicleSystem/OwnedVehicle.h"
#include "RacingVehicleSystem/VehicleDefinition.h"
#include "SChallengePromptWidget.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerController.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"

ACourseGameMode::ACourseGameMode()
{
    // DefaultPawnClass starts null; InitGame sets it from the player's
    // selected vehicle before any player connects.
    DefaultPawnClass = nullptr;
}

void ACourseGameMode::InitGame(const FString& MapName,
                               const FString& Options,
                               FString&       ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    URacingGameInstance* GI = URacingGameInstance::Get(this);
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("CourseGameMode::InitGame � no GameInstance"));
        return;
    }

    UVehicleInventory* Inventory = GI->GetVehicleInventory();
    UOwnedVehicle*     Vehicle   = Inventory ? Inventory->GetCurrentVehicle() : nullptr;

    if (Vehicle && Vehicle->Definition)
    {
        UClass* PawnClass = Vehicle->Definition->PawnClass.LoadSynchronous();
        if (PawnClass)
        {
            DefaultPawnClass = PawnClass;
            UE_LOG(LogTemp, Warning,
                TEXT("CourseGameMode::InitGame � DefaultPawnClass set to '%s'"),
                *PawnClass->GetName());
            return;
        }
    }

    UE_LOG(LogTemp, Error,
        TEXT("CourseGameMode::InitGame � could not resolve vehicle pawn class. ")
        TEXT("Make sure the player has purchased a vehicle and DA_VehicleDefinition.PawnClass is set."));
}

void ACourseGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("=== CourseGameMode::BeginPlay ==="));

    // Explicitly restore game input in case UIOnly mode carried over from hub.
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }

    SpawnManager = NewObject<UCourseNPCSpawnManager>(this, TEXT("SpawnManager"));
    SpawnManager->MaxNPCsOnCourse = MaxNPCsOnCourse;

    SpawnNPCs();

    GetWorldTimerManager().SetTimer(DiagnosticTimerHandle,
        this, &ACourseGameMode::LogVehicleDiagnostics, 1.0f, false);
}

// ---------------------------------------------------------------------------
// NPC Spawning
// ---------------------------------------------------------------------------

void ACourseGameMode::SpawnNPCs()
{
    URacingGameInstance* GI = URacingGameInstance::Get(this);
    ACourseSplineActor*  SplineActor = FindCourseSplineActor();

    // Convert TObjectPtr array to raw pointer array for spawn manager
    TArray<UNPCRacerData*> RawRacers;
    RawRacers.Reserve(AllNPCRacers.Num());
    for (TObjectPtr<UNPCRacerData>& Ptr : AllNPCRacers)
    {
        if (Ptr) { RawRacers.Add(Ptr.Get()); }
    }

    TArray<FNPCSpawnEntry> SpawnList = SpawnManager->BuildSpawnList(RawRacers, GI);

    AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();

    for (const FNPCSpawnEntry& Entry : SpawnList)
    {
        if (!Entry.RacerData) { continue; }

        // Resolve patrol spline: prefer NPC's named preference, fall back to random
        URacingSplineComponent* Spline = nullptr;
        if (SplineActor)
        {
            if (!Entry.PatrolSplineName.IsNone())
            {
                Spline = SplineActor->GetSplineByName(Entry.PatrolSplineName);
            }
            if (!Spline)
            {
                Spline = SplineActor->GetRandomSpline();
            }
        }

        // Pick a random point along the spline as the spawn location
        FVector SpawnLocation = FVector(0,0,500); // fallback
        FRotator SpawnRotation = FRotator::ZeroRotator;
        if (Spline)
        {
            const float SplineLen = Spline->GetSplineLength();
            const float RandDist  = FMath::FRandRange(0.f, SplineLen);
            SpawnLocation  = Spline->GetLocationAtDistance(RandDist);
            SpawnLocation.Z += NPCSpawnZOffset;
            SpawnRotation  = Spline->GetDirectionAtDistance(RandDist).Rotation();
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ANPCPatrolActor* PatrolActor = GetWorld()->SpawnActor<ANPCPatrolActor>(
            ANPCPatrolActor::StaticClass(),
            SpawnLocation, SpawnRotation, Params);

        if (!PatrolActor) { continue; }

        PatrolActor->Initialise(Entry.RacerData, Spline, PlayerPawn);

        // Bind challenge delegate using a lambda that captures the game mode
        PatrolActor->OnChallenged.BindUObject(this, &ACourseGameMode::OnNPCChallenged);

        PatrolActors.Add(PatrolActor);
    }

    UE_LOG(LogTemp, Log, TEXT("ACourseGameMode: spawned %d NPC patrol actors"),
        PatrolActors.Num());
}

// ---------------------------------------------------------------------------
// Challenge flow
// ---------------------------------------------------------------------------

void ACourseGameMode::OnNPCChallenged(ANPCPatrolActor* Challenger)
{
    // Ignore if a battle is already in progress
    if (ActiveBattleNPC) { return; }
    // Ignore if prompt is already showing for another NPC
    if (PendingChallenge) { return; }

    PendingChallenge = Challenger;
    ShowChallengePrompt(Challenger);
}

void ACourseGameMode::OnChallengeResponse(bool bAccepted)
{
    HideChallengePrompt();

    if (!PendingChallenge) { return; }

    if (bAccepted)
    {
        ActiveBattleNPC = PendingChallenge;
        ActiveBattleNPC->StartBattle();

        // Restore game input (player is now racing)
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }
    }

    PendingChallenge = nullptr;
}

void ACourseGameMode::OnBattleEnded()
{
    if (ActiveBattleNPC)
    {
        ActiveBattleNPC->EndBattle();
        ActiveBattleNPC = nullptr;
    }

    HideChallengePrompt();
}

// ---------------------------------------------------------------------------
// Widget helpers
// ---------------------------------------------------------------------------

void ACourseGameMode::ShowChallengePrompt(ANPCPatrolActor* Challenger)
{
    if (!GEngine || !GEngine->GameViewport) { return; }

    ChallengeWidget = SNew(SChallengePromptWidget)
        .RacerData(Challenger->GetRacerData())
        .OnResponse(FOnChallengeResponse::CreateUObject(
            this, &ACourseGameMode::OnChallengeResponse));

    GEngine->GameViewport->AddViewportWidgetContent(
        ChallengeWidget.ToSharedRef(), /* ZOrder */ 20);

    // Show cursor so the player can click the buttons
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
}

void ACourseGameMode::HideChallengePrompt()
{
    if (ChallengeWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(
            ChallengeWidget.ToSharedRef());
    }
    ChallengeWidget.Reset();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

AVehicleExamplePawn* ACourseGameMode::GetPlayerVehiclePawn() const
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    return PC ? Cast<AVehicleExamplePawn>(PC->GetPawn()) : nullptr;
}

ACourseSplineActor* ACourseGameMode::FindCourseSplineActor() const
{
    for (TActorIterator<ACourseSplineActor> It(GetWorld()); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

void ACourseGameMode::LogVehicleDiagnostics()
{
    UE_LOG(LogTemp, Warning, TEXT("===== COURSE DIAG ====="));
    UE_LOG(LogTemp, Warning, TEXT("DIAG: World gravity Z = %.1f"), GetWorld()->GetDefaultGravityZ());

    // --- Player controller state ---
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("DIAG: PlayerController='%s' | PossessedPawn='%s' | ShowCursor=%d"),
            *PC->GetClass()->GetName(),
            PC->GetPawn() ? *PC->GetPawn()->GetClass()->GetName() : TEXT("NONE"),
            PC->bShowMouseCursor);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DIAG: No PlayerController found!"));
    }

    // --- Player pawn ---
    AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
    if (!PlayerPawn)
    {
        UE_LOG(LogTemp, Error,
            TEXT("DIAG: No player pawn possessed. SpawnPlayerVehicle likely returned early."));
    }
    else
    {
        const FVector Pos = PlayerPawn->GetActorLocation();
        const FVector Vel = PlayerPawn->GetVelocity();
        const bool bSimulating = PlayerPawn->GetMesh()
            ? PlayerPawn->GetMesh()->IsSimulatingPhysics() : false;

        UChaosWheeledVehicleMovementComponent* Move =
            Cast<UChaosWheeledVehicleMovementComponent>(
                PlayerPawn->GetVehicleMovement());

        const bool bOnGround = Move ? Move->IsMovingOnGround() : false;

        UE_LOG(LogTemp, Warning,
            TEXT("DIAG: Player pawn '%s' | Pos=(%.0f, %.0f, %.0f) | Vel=(%.1f, %.1f, %.1f) | SimPhys=%d | OnGround=%d"),
            *PlayerPawn->GetClass()->GetName(),
            Pos.X, Pos.Y, Pos.Z,
            Vel.X, Vel.Y, Vel.Z,
            bSimulating, bOnGround);

        // Draw a debug sphere at spawn location for 10 seconds
        DrawDebugSphere(GetWorld(), Pos, 80.f, 12,
            bOnGround ? FColor::Green : FColor::Red, false, 10.f);
    }

    // --- NPC pawns ---
    for (TObjectPtr<ANPCPatrolActor>& Patrol : PatrolActors)
    {
        if (!Patrol) { continue; }
        AVehicleExamplePawn* NPCPawn = Patrol->GetNPCPawn();
        if (!NPCPawn)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("DIAG: PatrolActor '%s' has no pawn (SpawnNPCPawn failed)."),
                *Patrol->GetName());
            continue;
        }
        const FVector NPos = NPCPawn->GetActorLocation();
        const bool bNSim  = NPCPawn->GetMesh()
            ? NPCPawn->GetMesh()->IsSimulatingPhysics() : false;
        UE_LOG(LogTemp, Warning,
            TEXT("DIAG: NPC '%s' | Pos=(%.0f, %.0f, %.0f) | SimPhys=%d"),
            *NPCPawn->GetClass()->GetName(), NPos.X, NPos.Y, NPos.Z, bNSim);
        Patrol->LogAIDiagnostics();
        DrawDebugSphere(GetWorld(), NPos, 80.f, 12, FColor::Yellow, false, 10.f);
    }

    // --- PlayerStart location ---
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("DIAG: PlayerStart at (%.0f, %.0f, %.0f)"),
            It->GetActorLocation().X,
            It->GetActorLocation().Y,
            It->GetActorLocation().Z);
        break;
    }

    UE_LOG(LogTemp, Warning, TEXT("===== END DIAG ====="));
}
