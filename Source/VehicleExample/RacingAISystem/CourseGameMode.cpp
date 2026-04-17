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
#include "SInputDebugWidget.h"
#include "SRaceHUDWidget.h"
#include "SRaceResultWidget.h"
#include "RacingCharacterSystem/NPCRacerData.h"
#include "RacingCharacterSystem/PlayerPerkManager.h"
#include "RacingCharacterSystem/CharacterPerkState.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerController.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "RacingAIController.h"

ACourseGameMode::ACourseGameMode()
{
    // DefaultPawnClass starts null; InitGame sets it from the player's
    // selected vehicle before any player connects.
    DefaultPawnClass = nullptr;

    PrimaryActorTick.bCanEverTick = true;
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

    // Add the input debug overlay once the pawn is available.
    // We defer one frame so the player pawn is guaranteed to be possessed.
    GetWorldTimerManager().SetTimerForNextTick([this]()
    {
        AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
        if (PlayerPawn && GEngine && GEngine->GameViewport)
        {
            InputDebugWidget = SNew(SInputDebugWidget)
                .PlayerPawn(PlayerPawn);
            GEngine->GameViewport->AddViewportWidgetContent(
                InputDebugWidget.ToSharedRef(), /*ZOrder=*/5);
        }
    });
}

// ---------------------------------------------------------------------------
// NPC Spawning
// ---------------------------------------------------------------------------

void ACourseGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (InputDebugWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(InputDebugWidget.ToSharedRef());
    }
    InputDebugWidget.Reset();

    HideRaceHUD();
    HideRaceResult();

    Super::EndPlay(EndPlayReason);
}

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

        // Bind challenge delegates
        PatrolActor->OnChallenged.BindUObject(this, &ACourseGameMode::OnNPCChallenged);
        PatrolActor->OnChallengeLeft.BindUObject(this, &ACourseGameMode::OnNPCChallengeLeft);

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

void ACourseGameMode::OnNPCChallengeLeft(ANPCPatrolActor* Challenger)
{
    // Only dismiss if this is the NPC whose prompt is currently showing
    if (Challenger != PendingChallenge) { return; }

    HideChallengePrompt();
    PendingChallenge = nullptr;

    // Restore game-only input (cursor was shown when prompt appeared)
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void ACourseGameMode::OnChallengeResponse(bool bAccepted)
{
    HideChallengePrompt();

    if (!PendingChallenge) { return; }

    if (bAccepted)
    {
        ActiveBattleNPC = PendingChallenge;
        ActiveBattleNPC->StartBattle();

        // Initialise health and show race HUD
        InitBattleHealth(ActiveBattleNPC);
        ShowRaceHUD(ActiveBattleNPC);
        BattleStartTime = GetWorld()->GetTimeSeconds();
        bBattleActive   = true;

        // Enable hit events and bind collision handlers on both pawns
        AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
        AVehicleExamplePawn* NPCPawn    = ActiveBattleNPC->GetNPCPawn();

        if (PlayerPawn)
        {
            if (USkeletalMeshComponent* Mesh = PlayerPawn->GetMesh())
            {
                Mesh->SetNotifyRigidBodyCollision(true);
            }
            PlayerPawn->OnActorHit.AddDynamic(this, &ACourseGameMode::OnPlayerPawnHit);
        }
        if (NPCPawn)
        {
            if (USkeletalMeshComponent* Mesh = NPCPawn->GetMesh())
            {
                Mesh->SetNotifyRigidBodyCollision(true);
            }
            NPCPawn->OnActorHit.AddDynamic(this, &ACourseGameMode::OnNPCPawnHit);
        }
    }

    // Always restore game-only input when prompt is dismissed
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }

    PendingChallenge = nullptr;
}

void ACourseGameMode::OnBattleEnded()
{
    bBattleActive = false;

    // Unbind collision handlers and disable hit events
    AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
    if (PlayerPawn)
    {
        PlayerPawn->OnActorHit.RemoveDynamic(this, &ACourseGameMode::OnPlayerPawnHit);
        if (USkeletalMeshComponent* Mesh = PlayerPawn->GetMesh())
        {
            Mesh->SetNotifyRigidBodyCollision(false);
        }
    }

    if (ActiveBattleNPC)
    {
        AVehicleExamplePawn* NPCPawn = ActiveBattleNPC->GetNPCPawn();
        if (NPCPawn)
        {
            NPCPawn->OnActorHit.RemoveDynamic(this, &ACourseGameMode::OnNPCPawnHit);
            if (USkeletalMeshComponent* Mesh = NPCPawn->GetMesh())
            {
                Mesh->SetNotifyRigidBodyCollision(false);
            }
        }
        ActiveBattleNPC->EndBattle();
        ActiveBattleNPC = nullptr;
    }

    HideRaceHUD();
    HideRaceResult();
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
// Battle health initialisation
// ---------------------------------------------------------------------------

void ACourseGameMode::InitBattleHealth(ANPCPatrolActor* NPC)
{
    // NPC max HP from their data asset base stats
    UNPCRacerData* RacerData = NPC ? NPC->GetRacerData() : nullptr;
    NPCMaxHP     = RacerData ? FMath::Max(1.f, static_cast<float>(RacerData->BaseStats.Health)) : 100.f;
    NPCCurrentHP = NPCMaxHP;

    // Player max HP: base 100 + Health perk contributions
    PlayerMaxHP = static_cast<float>(ComputePlayerStats().Health);
    if (PlayerMaxHP <= 0.f) { PlayerMaxHP = 100.f; }
    PlayerCurrentHP = PlayerMaxHP;
}

FDriverStatBlock ACourseGameMode::ComputePlayerStats() const
{
    FDriverStatBlock Base;
    Base.Health = 100;

    URacingGameInstance* GI = URacingGameInstance::Get(this);
    if (!GI || !GI->GetPerkManager() || !GI->GetPerkManager()->PerkState)
    {
        return Base;
    }

    TArray<UPerkData*> AllPerks;
    for (const TObjectPtr<UPerkData>& P : GI->GetPerkManager()->AllPerks)
    {
        if (P) { AllPerks.Add(P.Get()); }
    }

    return GI->GetPerkManager()->PerkState->ComputeStats(AllPerks, Base);
}

// ---------------------------------------------------------------------------
// Race HUD widget
// ---------------------------------------------------------------------------

void ACourseGameMode::ShowRaceHUD(ANPCPatrolActor* NPC)
{
    if (!GEngine || !GEngine->GameViewport) { return; }
    HideRaceHUD();

    const FText PlayerName = NSLOCTEXT("RaceHUD", "PlayerLabel", "PLAYER");
    const FText NPCName    = NPC && NPC->GetRacerData()
        ? NPC->GetRacerData()->RacerName
        : NSLOCTEXT("RaceHUD", "NPCFallback", "OPPONENT");

    // Capture raw pointers for attribute lambdas (game mode outlives the widget)
    ACourseGameMode* GM = this;

    RaceHUDWidget = SNew(SRaceHUDWidget)
        .PlayerName(PlayerName)
        .NPCName(NPCName)
        .PlayerHealthFraction_Lambda([GM]() -> float
        {
            return GM->PlayerMaxHP > 0.f
                ? FMath::Clamp(GM->PlayerCurrentHP / GM->PlayerMaxHP, 0.f, 1.f)
                : 0.f;
        })
        .NPCHealthFraction_Lambda([GM]() -> float
        {
            return GM->NPCMaxHP > 0.f
                ? FMath::Clamp(GM->NPCCurrentHP / GM->NPCMaxHP, 0.f, 1.f)
                : 0.f;
        })
        .TimerText_Lambda([GM]() -> FText
        {
            if (!GM->bBattleActive) { return FText::FromString(TEXT("0:00")); }
            const float  Elapsed  = GM->GetWorld()->GetTimeSeconds() - GM->BattleStartTime;
            const int32  TotalSec = FMath::FloorToInt(Elapsed);
            const int32  Min      = TotalSec / 60;
            const int32  Sec      = TotalSec % 60;
            return FText::FromString(FString::Printf(TEXT("%d:%02d"), Min, Sec));
        });

    GEngine->GameViewport->AddViewportWidgetContent(
        RaceHUDWidget.ToSharedRef(), /*ZOrder=*/10);
}

void ACourseGameMode::HideRaceHUD()
{
    if (RaceHUDWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(RaceHUDWidget.ToSharedRef());
    }
    RaceHUDWidget.Reset();
}

// ---------------------------------------------------------------------------
// Race result popup
// ---------------------------------------------------------------------------

void ACourseGameMode::ShowRaceResult(bool bPlayerWon)
{
    if (!GEngine || !GEngine->GameViewport) { return; }
    HideRaceResult();
    HideRaceHUD();

    const float Elapsed = GetWorld()->GetTimeSeconds() - BattleStartTime;

    RaceResultWidget = SNew(SRaceResultWidget)
        .bPlayerWon(bPlayerWon)
        .ElapsedSeconds(Elapsed)
        .OnContinue(FOnRaceResultContinue::CreateUObject(
            this, &ACourseGameMode::OnBattleEnded));

    GEngine->GameViewport->AddViewportWidgetContent(
        RaceResultWidget.ToSharedRef(), /*ZOrder=*/30);

    // Show cursor so the player can click CONTINUE
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeGameAndUI());
    }
}

void ACourseGameMode::HideRaceResult()
{
    if (RaceResultWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(RaceResultWidget.ToSharedRef());
    }
    RaceResultWidget.Reset();
}

// ---------------------------------------------------------------------------
// Tick — distance-based HP drain
// ---------------------------------------------------------------------------

void ACourseGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Keybind: F (keyboard) or Gamepad A to accept a pending challenge prompt
    if (PendingChallenge)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && (PC->WasInputKeyJustPressed(EKeys::F) ||
                   PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)))
        {
            OnChallengeResponse(true);
            return; // don't also run the distance drain this frame
        }
    }

    if (!bBattleActive || !ActiveBattleNPC) { return; }

    ARacingAIController* AIC = ActiveBattleNPC->GetAIController();
    if (!AIC) { return; }

    // DistanceToPlayerCm = PlayerSplineDist - NPCSplineDist
    // Positive  → player is ahead of NPC
    // Negative  → player is behind NPC
    const float SignedGap = AIC->GetContext().DistanceToPlayerCm;

    if (SignedGap < -DistanceDrainThresholdCm)
    {
        // Player is more than 26 yards behind — drain player HP
        PlayerCurrentHP = FMath::Max(0.f,
            PlayerCurrentHP - DistanceDrainRatePerSecond * DeltaSeconds);

        if (PlayerCurrentHP <= 0.f)
        {
            TriggerBattleEnd(/*bPlayerWon=*/false);
        }
    }
    else if (SignedGap > DistanceDrainThresholdCm)
    {
        // NPC is more than 26 yards behind — drain NPC HP
        NPCCurrentHP = FMath::Max(0.f,
            NPCCurrentHP - DistanceDrainRatePerSecond * DeltaSeconds);

        if (NPCCurrentHP <= 0.f)
        {
            TriggerBattleEnd(/*bPlayerWon=*/true);
        }
    }
}

// ---------------------------------------------------------------------------
// Battle end trigger (called when HP reaches 0)
// ---------------------------------------------------------------------------

void ACourseGameMode::TriggerBattleEnd(bool bPlayerWon)
{
    if (!bBattleActive) { return; }
    bBattleActive = false;

    // Unbind hit events immediately so no more damage is dealt
    AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
    if (PlayerPawn)
    {
        PlayerPawn->OnActorHit.RemoveDynamic(this, &ACourseGameMode::OnPlayerPawnHit);
    }
    if (ActiveBattleNPC)
    {
        AVehicleExamplePawn* NPCPawn = ActiveBattleNPC->GetNPCPawn();
        if (NPCPawn)
        {
            NPCPawn->OnActorHit.RemoveDynamic(this, &ACourseGameMode::OnNPCPawnHit);
        }
    }

    ShowRaceResult(bPlayerWon);
}

// ---------------------------------------------------------------------------
// Collision damage handlers
// ---------------------------------------------------------------------------

void ACourseGameMode::OnPlayerPawnHit(AActor*          SelfActor,
                                       AActor*          OtherActor,
                                       FVector          NormalImpulse,
                                       const FHitResult& Hit)
{
    if (!bBattleActive || !ActiveBattleNPC) { return; }

    const float ImpulseMag = NormalImpulse.Size();
    float Damage = 0.f;

    if (OtherActor == Cast<AActor>(ActiveBattleNPC->GetNPCPawn()))
    {
        // Vehicle-to-vehicle collision: player takes damage
        Damage = FMath::Max(MinCollisionDamage, ImpulseMag / CollisionDamageScale);
        UE_LOG(LogTemp, Verbose,
            TEXT("RaceHUD: Player hit by NPC (impulse=%.0f) — HP %.1f / %.1f"),
            ImpulseMag, PlayerCurrentHP, PlayerMaxHP);
    }
    else if (Cast<APawn>(OtherActor) == nullptr
             && ImpulseMag >= MinWallImpulse
             && Hit.ImpactNormal.Z < 0.7f)
    {
        // Wall/barrier collision: player takes damage (capped to avoid one-shots)
        Damage = FMath::Clamp(ImpulseMag / WallCollisionDamageScale,
                              MinCollisionDamage, MaxWallDamagePerHit);
        UE_LOG(LogTemp, Verbose,
            TEXT("RaceHUD: Player hit wall (impulse=%.0f) — HP %.1f / %.1f"),
            ImpulseMag, PlayerCurrentHP, PlayerMaxHP);
    }
    else
    {
        return; // Road surface or irrelevant contact
    }

    PlayerCurrentHP = FMath::Max(0.f, PlayerCurrentHP - Damage);
    if (PlayerCurrentHP <= 0.f)
    {
        TriggerBattleEnd(/*bPlayerWon=*/false);
    }
}

void ACourseGameMode::OnNPCPawnHit(AActor*          SelfActor,
                                    AActor*          OtherActor,
                                    FVector          NormalImpulse,
                                    const FHitResult& Hit)
{
    if (!bBattleActive) { return; }

    AVehicleExamplePawn* PlayerPawn = GetPlayerVehiclePawn();
    const float ImpulseMag = NormalImpulse.Size();
    float Damage = 0.f;

    if (OtherActor == Cast<AActor>(PlayerPawn))
    {
        // Vehicle-to-vehicle collision: NPC takes damage
        Damage = FMath::Max(MinCollisionDamage, ImpulseMag / CollisionDamageScale);
        UE_LOG(LogTemp, Verbose,
            TEXT("RaceHUD: NPC hit by Player (impulse=%.0f) — HP %.1f / %.1f"),
            ImpulseMag, NPCCurrentHP, NPCMaxHP);
    }
    else if (Cast<APawn>(OtherActor) == nullptr
             && ImpulseMag >= MinWallImpulse
             && Hit.ImpactNormal.Z < 0.7f)
    {
        // Wall/barrier collision: NPC takes damage (capped to avoid one-shots)
        Damage = FMath::Clamp(ImpulseMag / WallCollisionDamageScale,
                              MinCollisionDamage, MaxWallDamagePerHit);
        UE_LOG(LogTemp, Verbose,
            TEXT("RaceHUD: NPC hit wall (impulse=%.0f) — HP %.1f / %.1f"),
            ImpulseMag, NPCCurrentHP, NPCMaxHP);
    }
    else
    {
        return; // Road surface or irrelevant contact
    }

    NPCCurrentHP = FMath::Max(0.f, NPCCurrentHP - Damage);
    if (NPCCurrentHP <= 0.f)
    {
        TriggerBattleEnd(/*bPlayerWon=*/true);
    }
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
