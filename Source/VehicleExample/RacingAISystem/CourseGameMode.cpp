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
#include "GameFramework/PlayerStart.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

ACourseGameMode::ACourseGameMode()
{
    // Suppress the engine auto-spawning a default pawn.
    // We spawn the correct vehicle pawn ourselves in BeginPlay.
    DefaultPawnClass = nullptr;
}

void ACourseGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Spawn the player's selected vehicle first so GetPlayerVehiclePawn()
    // returns a valid pawn when SpawnNPCs() runs.
    SpawnPlayerVehicle();

    SpawnManager = NewObject<UCourseNPCSpawnManager>(this, TEXT("SpawnManager"));
    SpawnManager->MaxNPCsOnCourse = MaxNPCsOnCourse;

    SpawnNPCs();
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
        FVector SpawnLocation = FVector::ZeroVector; // fallback
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

AVehicleExamplePawn* ACourseGameMode::SpawnPlayerVehicle()
{
    URacingGameInstance* GI = URacingGameInstance::Get(this);
    if (!GI) { return nullptr; }

    UVehicleInventory* Inventory = GI->GetVehicleInventory();
    if (!Inventory) { return nullptr; }

    UOwnedVehicle* OwnedVehicle = Inventory->GetCurrentVehicle();
    if (!OwnedVehicle || !OwnedVehicle->Definition)
    {
        UE_LOG(LogTemp, Error,
            TEXT("ACourseGameMode: No current vehicle selected. ")
            TEXT("Make sure the player has purchased a vehicle and CurrentVehicleID is set."));
        return nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("ACourseGameMode: Spawning player vehicle '%s'."),
        *OwnedVehicle->Definition->VehicleID.ToString());

    // Load the pawn class synchronously
    UClass* PawnClass = OwnedVehicle->Definition->PawnClass.LoadSynchronous();
    if (!PawnClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ACourseGameMode: PawnClass not set on vehicle definition '%s'."),
            *OwnedVehicle->Definition->VehicleID.ToString());
        return nullptr;
    }

    // Find the first PlayerStart in the level
    FTransform SpawnTransform = FTransform::Identity;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        SpawnTransform = It->GetActorTransform();
        // Apply Z offset so Chaos suspension has room to settle
        FVector Loc = SpawnTransform.GetLocation();
        Loc.Z += PlayerSpawnZOffset;
        SpawnTransform.SetLocation(Loc);
        break;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AVehicleExamplePawn* PlayerPawn = GetWorld()->SpawnActor<AVehicleExamplePawn>(
        PawnClass, SpawnTransform, Params);

    if (!PlayerPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("ACourseGameMode: Failed to spawn player pawn."));
        return nullptr;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->Possess(PlayerPawn);

        // Add the vehicle Enhanced Input Mapping Context.
        // This replaces what the Blueprint BeginPlay normally does when
        // the pawn is possessed through the standard flow.
        if (VehicleInputMappingContext)
        {
            if (ULocalPlayer* LP = PC->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                    LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    Subsystem->AddMappingContext(VehicleInputMappingContext, 0);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ACourseGameMode: Spawned player pawn '%s'."),
        *PawnClass->GetName());

    return PlayerPawn;
}
