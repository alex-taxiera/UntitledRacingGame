// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPCPatrolActor.generated.h"

class URacingSplineComponent;
class UNPCRacerData;
class ARacingAIController;
class AVehicleExamplePawn;

DECLARE_DELEGATE_OneParam(FOnNPCChallenged, class ANPCPatrolActor*);

/**
 * ANPCPatrolActor
 *
 * Manages one NPC on the course: spawns its pawn, drives the AI controller
 * between idle patrol and race mode, and detects when the player is close
 * enough to issue a challenge.
 *
 * Lifecycle:
 *   1. ACourseGameMode spawns this actor and calls Initialise().
 *   2. Initialise() spawns the vehicle pawn, possesses it with
 *      ARacingAIController, and calls StartIdle().
 *   3. When the player enters the ChallengeRadius sphere, OnChallenged fires.
 *      ACourseGameMode shows the challenge prompt.
 *   4. On player accept: ACourseGameMode calls StartBattle() on this actor,
 *      which transitions the controller to race mode.
 *   5. On race end: ACourseGameMode calls EndBattle(), returning NPC to idle.
 */
UCLASS(BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API ANPCPatrolActor : public AActor
{
    GENERATED_BODY()

public:

    ANPCPatrolActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    // -----------------------------------------------------------------------
    // Setup
    // -----------------------------------------------------------------------

    /**
     * Initialises the patrol actor with its racer data and patrol spline.
     * Must be called after spawning, before BeginPlay completes (or call
     * it immediately after spawning and before the first tick).
     *
     * @param InRacerData    The NPC data asset defining stats, AI config etc.
     * @param InPatrolSpline The spline this NPC follows while idle.
     * @param InPlayerPawn   The player's pawn (used by race AI for targeting).
     */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void Initialise(UNPCRacerData*         InRacerData,
                    URacingSplineComponent* InPatrolSpline,
                    AVehicleExamplePawn*    InPlayerPawn);

    // -----------------------------------------------------------------------
    // Race lifecycle
    // -----------------------------------------------------------------------

    /**
     * Transitions this NPC from idle patrol to race mode.
     * Called by ACourseGameMode after the player accepts a challenge.
     */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void StartBattle();

    /**
     * Returns this NPC to idle patrol after a battle ends.
     */
    UFUNCTION(BlueprintCallable, Category = "NPC")
    void EndBattle();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    UNPCRacerData* GetRacerData() const { return RacerData; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    AVehicleExamplePawn* GetNPCPawn() const { return NPCPawn; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    bool IsInBattle() const { return bInBattle; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC")
    ARacingAIController* GetAIController() const { return AIController; }
    /** Logs AI controller internal state � call from diagnostics only. */
    void LogAIDiagnostics() const;

    // -----------------------------------------------------------------------
    // Delegate � bind in ACourseGameMode
    // -----------------------------------------------------------------------

    /** Fired when the player enters the challenge trigger radius. */
    FOnNPCChallenged OnChallenged;

    /** Fired when the player exits the challenge trigger radius while the prompt is pending. */
    FOnNPCChallenged OnChallengeLeft;

    // -----------------------------------------------------------------------
    // Configurable properties
    // -----------------------------------------------------------------------

    /**
     * Radius (cm) within which the player pawn triggers the challenge prompt.
     * Default 1500 cm = 15 m.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Challenge",
        meta = (ClampMin = "100.0"))
    float ChallengeRadius = 1500.f;

private:

    UPROPERTY()
    TObjectPtr<UNPCRacerData> RacerData;

    UPROPERTY()
    TObjectPtr<URacingSplineComponent> PatrolSpline;

    UPROPERTY()
    TObjectPtr<AVehicleExamplePawn> PlayerPawn;

    UPROPERTY()
    TObjectPtr<AVehicleExamplePawn> NPCPawn;

    UPROPERTY()
    TObjectPtr<ARacingAIController> AIController;

    bool bInitialised   = false;
    bool bInBattle      = false;
    bool bPlayerInRange = false;

    /** Spawns the vehicle pawn at this actor's location and possesses it. */
    void SpawnNPCPawn();
};
