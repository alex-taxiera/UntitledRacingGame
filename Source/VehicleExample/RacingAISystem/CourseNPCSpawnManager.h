// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CourseNPCSpawnManager.generated.h"

class UNPCRacerData;
class URacingGameInstance;

/**
 * FNPCSpawnEntry
 *
 * One resolved entry in the final spawn list produced by
 * UCourseNPCSpawnManager::BuildSpawnList().
 */
USTRUCT(BlueprintType)
struct FNPCSpawnEntry
{
    GENERATED_BODY()

    /** The NPC data asset to spawn. */
    UPROPERTY(BlueprintReadOnly, Category = "Spawn")
    TObjectPtr<UNPCRacerData> RacerData = nullptr;

    /**
     * Name of the patrol spline this NPC should use.
     * Taken from RacerData->PatrolSplineName; may be None if the NPC
     * has no preference (spawn manager assigns one at spawn time).
     */
    UPROPERTY(BlueprintReadOnly, Category = "Spawn")
    FName PatrolSplineName;
};

/**
 * UCourseNPCSpawnManager
 *
 * A UObject (not an actor) that lives on ACourseGameMode.
 * Responsible for:
 *   1. Evaluating spawn conditions on each UNPCRacerData in the global roster
 *   2. Building a weighted pool of eligible NPCs
 *   3. Selecting up to MaxNPCsOnCourse entries without repeats
 *   4. Returning the list to the game mode for actual pawn spawning
 *
 * Extending spawn conditions:
 *   Add a new ESpawnConditionType value in NPCRacerData.h and add a
 *   corresponding case in EvaluateCondition() below.  No other changes needed.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UCourseNPCSpawnManager : public UObject
{
    GENERATED_BODY()

public:

    /**
     * Maximum number of NPCs to place on the course at once.
     * Set by ACourseGameMode before calling BuildSpawnList().
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning",
        meta = (ClampMin = "1"))
    int32 MaxNPCsOnCourse = 20;

    /**
     * Builds the final list of NPCs to spawn.
     *
     * Algorithm:
     *   1. For each asset in AllRacers, evaluate all SpawnConditions.
     *      Any failing condition excludes the NPC from the pool.
     *   2. Perform weighted random selection without replacement until
     *      MaxNPCsOnCourse entries are chosen or the pool is exhausted.
     *
     * @param AllRacers   The full roster (e.g. from a designer-populated array).
     * @param GI          Game instance used to evaluate currency/story conditions.
     * @return            Up to MaxNPCsOnCourse FNPCSpawnEntry records.
     */
    UFUNCTION(BlueprintCallable, Category = "Spawning")
    TArray<FNPCSpawnEntry> BuildSpawnList(
        const TArray<UNPCRacerData*>& AllRacers,
        URacingGameInstance*          GI) const;

private:

    /**
     * Returns true if all of RacerData's SpawnConditions pass against GI.
     */
    bool AreConditionsMet(UNPCRacerData* RacerData,
                          URacingGameInstance* GI) const;
};
