// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CourseNPCSpawnManager.h"
#include "CourseGameMode.generated.h"

class ACourseSplineActor;
class ANPCPatrolActor;
class AVehicleExamplePawn;
class SChallengePromptWidget;
class UNPCRacerData;
class UInputMappingContext;

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

    virtual void BeginPlay() override;

    // -----------------------------------------------------------------------
    // Designer properties
    // -----------------------------------------------------------------------

    /**
     * Full NPC roster. Add all UNPCRacerData assets here in the Blueprint
     * subclass class defaults. The spawn manager selects MaxNPCsOnCourse
     * of them using weighted random selection.
     */
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

    /**
     * Z offset applied to the player pawn spawn position.
     * Chaos physics needs the car slightly above ground so the suspension
     * can compress to its rest position on the first frame.
     * Default 120 cm works for most vehicle setups.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course|Player")
    float PlayerSpawnZOffset = 120.f;

    /**
     * The Input Mapping Context to add for the player after possession.
     * Set this to your vehicle IMC asset (e.g. IMC_Vehicle) in BP_CourseGameMode.
     * Without this, Enhanced Input actions won't fire and the car won't respond.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course|Player")
    TObjectPtr<UInputMappingContext> VehicleInputMappingContext;

    // -----------------------------------------------------------------------
    // Battle lifecycle (call from race result screen / external systems)
    // -----------------------------------------------------------------------

    /**
     * Call this when a race battle finishes (win or lose) to return the NPC
     * to idle patrol and restore player input.
     */
    UFUNCTION(BlueprintCallable, Category = "Course")
    void OnBattleEnded();

private:

    UPROPERTY()
    TObjectPtr<UCourseNPCSpawnManager> SpawnManager;

    UPROPERTY()
    TArray<TObjectPtr<ANPCPatrolActor>> PatrolActors;

    /** NPC whose trigger the player just entered — awaiting response. */
    UPROPERTY()
    TObjectPtr<ANPCPatrolActor> PendingChallenge;

    /** NPC currently in an active race battle with the player. */
    UPROPERTY()
    TObjectPtr<ANPCPatrolActor> ActiveBattleNPC;

    TSharedPtr<SChallengePromptWidget> ChallengeWidget;

    // -----------------------------------------------------------------------
    // Internal
    // -----------------------------------------------------------------------

    void SpawnNPCs();

    /** Bound to every ANPCPatrolActor::OnChallenged delegate. */
    void OnNPCChallenged(ANPCPatrolActor* Challenger);

    /** Bound to SChallengePromptWidget::OnResponse. */
    void OnChallengeResponse(bool bAccepted);

    void ShowChallengePrompt(ANPCPatrolActor* Challenger);
    void HideChallengePrompt();

    AVehicleExamplePawn* GetPlayerVehiclePawn() const;
    ACourseSplineActor*  FindCourseSplineActor() const;

    /**
     * Reads CurrentVehicle from the game instance, synchronously loads its
     * PawnClass, spawns it at the first PlayerStart in the level, and
     * possesses it with the first player controller.
     * Returns the spawned pawn or nullptr on failure.
     */
    AVehicleExamplePawn* SpawnPlayerVehicle();
};
