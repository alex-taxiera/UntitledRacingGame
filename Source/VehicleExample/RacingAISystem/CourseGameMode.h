// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework\GameModeBase.h"
#include "CourseNPCSpawnManager.h"
#include "RacingCharacterSystem/PerkTypes.h"
#include "CourseGameMode.generated.h"

class ACourseSplineActor;
class ANPCPatrolActor;
class AVehicleExamplePawn;
class SChallengePromptWidget;
class SInputDebugWidget;
class SRaceHUDWidget;
class SRaceResultWidget;
class UNPCRacerData;

/**
 * ACourseGameMode
 *
 * Game mode for the open-world course level.
 *
 * Responsibilities:
 *   - Finds the ACourseSplineActor in the level at BeginPlay
 *   - Builds a spawn list via UCourseNPCSpawnManager
 *   - Spawns ANPCPatrolActor instances along their assigned splines
 *   - Listens for challenge triggers; shows SChallengePromptWidget
 *   - On player accept: calls ANPCPatrolActor::StartBattle()
 *   - On race end (called externally): calls EndBattle() and removes prompt
 *
 * How to use in the editor:
 *   1. Set WorldSettings -> GameMode Override to ACourseGameMode
 *      (or a Blueprint subclass BP_CourseGameMode).
 *   2. Place one ACourseSplineActor in the level and add spline names.
 *   3. In BP_CourseGameMode Class Defaults populate AllNPCRacers with
 *      your NPC data assets and adjust MaxNPCsOnCourse.
 */
UCLASS(BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API ACourseGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    ACourseGameMode();

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Called when a new player controller logs into the server. */
    virtual void PostLogin(APlayerController* NewPlayer) override;

    UFUNCTION(BlueprintCallable, Category = "Course")
    void OnBattleEnded();


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course|NPCs")
    TArray<TObjectPtr<UNPCRacerData>> AllNPCRacers;

    /** Maximum number of NPC patrol actors alive on the course at once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course|NPCs",
        meta = (ClampMin = "1"))
    int32 MaxNPCsOnCourse = 20;

    /**
     * Z offset applied to each NPC spawn position along their spline.
     * Increase if vehicles spawn clipped into the ground mesh.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course|NPCs")
    float NPCSpawnZOffset = 100.f;

    // -----------------------------------------------------------------------
private:

    UPROPERTY()
    TObjectPtr<UCourseNPCSpawnManager> SpawnManager;

    UPROPERTY()
    TArray<TObjectPtr<ANPCPatrolActor>> PatrolActors;

    /** NPC whose trigger the player just entered � awaiting response. */
    UPROPERTY()
    TObjectPtr<ANPCPatrolActor> PendingChallenge;

    /** NPC currently in an active race battle with the player. */
    UPROPERTY()
    TObjectPtr<ANPCPatrolActor> ActiveBattleNPC;

    TSharedPtr<SChallengePromptWidget> ChallengeWidget;
    TSharedPtr<SInputDebugWidget>      InputDebugWidget;
    TSharedPtr<SRaceHUDWidget>         RaceHUDWidget;
    TSharedPtr<SRaceResultWidget>      RaceResultWidget;
    FTimerHandle DiagnosticTimerHandle;

    // -----------------------------------------------------------------------
    // Battle health & damage
    // -----------------------------------------------------------------------

    float PlayerCurrentHP  = 0.f;
    float PlayerMaxHP      = 100.f;
    float NPCCurrentHP     = 0.f;
    float NPCMaxHP         = 100.f;
    float BattleStartTime  = 0.f;
    bool  bBattleActive    = false;

    /**
     * Impulse magnitude (cm/s * kg) divided by this gives HP damage per hit.
     * Tune in the Blueprint subclass to adjust combat feel.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle")
    float CollisionDamageScale = 4000.f;

    /** Minimum HP deducted from any valid vehicle-on-vehicle collision. */
    UPROPERTY(EditAnywhere, Category = "Course|Battle")
    float MinCollisionDamage = 2.f;

    /**
     * Impulse magnitude threshold (cm/s * kg) below which a non-pawn hit is
     * ignored — prevents road-surface micro-contacts from dealing wall damage.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle")
    float MinWallImpulse = 50000.f;

    /**
     * Impulse divisor used when a racer hits a wall or barrier.
     * Separate from CollisionDamageScale so wall hits can feel distinct.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle")
    float WallCollisionDamageScale = 6000.f;

    /**
     * Maximum HP a single wall collision can remove from a racer.
     * Prevents very high-speed impacts from dealing instant lethal damage.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle", meta = (ClampMin = "1.0"))
    float MaxWallDamagePerHit = 15.f;

    /**
     * Distance (in cm) the player must be behind the opponent before
     * HP drain kicks in. 2377.44 cm ≈ 26 yards.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle", meta = (ClampMin = "1.0"))
    float DistanceDrainThresholdCm = 2377.44f;

    /**
     * HP lost per second while a racer is beyond DistanceDrainThresholdCm behind
     * their opponent.
     */
    UPROPERTY(EditAnywhere, Category = "Course|Battle", meta = (ClampMin = "0.0"))
    float DistanceDrainRatePerSecond = 5.f;

    void InitBattleHealth(ANPCPatrolActor* NPC);
    void ShowRaceHUD(ANPCPatrolActor* NPC);
    void HideRaceHUD();
    void ShowRaceResult(bool bPlayerWon);
    void HideRaceResult();
    void TriggerBattleEnd(bool bPlayerWon);

    /** Computes the player's resolved FDriverStatBlock from their perk state. */
    FDriverStatBlock ComputePlayerStats() const;

    UFUNCTION()
    void OnPlayerPawnHit(AActor* SelfActor, AActor* OtherActor,
                         FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void OnNPCPawnHit(AActor* SelfActor, AActor* OtherActor,
                      FVector NormalImpulse, const FHitResult& Hit);

    void SpawnNPCs();

    /** Bound to every ANPCPatrolActor::OnChallenged delegate. */
    void OnNPCChallenged(ANPCPatrolActor* Challenger);

    /** Bound to every ANPCPatrolActor::OnChallengeLeft delegate — dismisses the prompt if pending. */
    void OnNPCChallengeLeft(ANPCPatrolActor* Challenger);

    /** Bound to SChallengePromptWidget::OnResponse. */
    void OnChallengeResponse(bool bAccepted);

    void ShowChallengePrompt(ANPCPatrolActor* Challenger);
    void HideChallengePrompt();

    AVehicleExamplePawn* GetPlayerVehiclePawn() const;
    ACourseSplineActor*  FindCourseSplineActor() const;
    void LogVehicleDiagnostics();

    /**
     * Finds a good spawn transform for a newly-joined player.
     * Picks the spline point closest to the centroid of all existing players,
     * then offsets perpendicular to the spline so they don't overlap.
     */
    FTransform GetSpawnTransformForJoiningPlayer() const;
};
