// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework\GameModeBase.h"
#include "CourseNPCSpawnManager.h"
#include "CourseGameMode.generated.h"

class ACourseSplineActor;
class ANPCPatrolActor;
class AVehicleExamplePawn;
class SChallengePromptWidget;
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
    FTimerHandle DiagnosticTimerHandle;

    void SpawnNPCs();

    /** Bound to every ANPCPatrolActor::OnChallenged delegate. */
    void OnNPCChallenged(ANPCPatrolActor* Challenger);

    /** Bound to SChallengePromptWidget::OnResponse. */
    void OnChallengeResponse(bool bAccepted);

    void ShowChallengePrompt(ANPCPatrolActor* Challenger);
    void HideChallengePrompt();

    AVehicleExamplePawn* GetPlayerVehiclePawn() const;
    ACourseSplineActor*  FindCourseSplineActor() const;
    void LogVehicleDiagnostics();
};
