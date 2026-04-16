// Copyright Epic Games, Inc. All Rights Reserved.

#include "CourseNPCSpawnManager.h"
#include "NPCRacerData.h"
#include "RacingGameInstance.h"
#include "RacingVehicleSystem/VehicleInventory.h"
#include "RacingCharacterSystem/PlayerPerkManager.h"

// ---------------------------------------------------------------------------
// BuildSpawnList
// ---------------------------------------------------------------------------

TArray<FNPCSpawnEntry> UCourseNPCSpawnManager::BuildSpawnList(
    const TArray<UNPCRacerData*>& AllRacers,
    URacingGameInstance*          GI) const
{
    // --- Step 1: build weighted eligible pool ---
    struct FWeightedCandidate
    {
        UNPCRacerData* Racer;
        float          Weight;
    };

    TArray<FWeightedCandidate> Pool;
    Pool.Reserve(AllRacers.Num());

    for (UNPCRacerData* Racer : AllRacers)
    {
        if (!Racer) { continue; }
        if (!AreConditionsMet(Racer, GI)) { continue; }

        Pool.Add({ Racer, FMath::Max(0.01f, Racer->SpawnWeight) });
    }

    // --- Step 2: weighted random selection without replacement ---
    TArray<FNPCSpawnEntry> Result;
    Result.Reserve(FMath::Min(MaxNPCsOnCourse, Pool.Num()));

    const int32 TargetCount = FMath::Min(MaxNPCsOnCourse, Pool.Num());

    while (Result.Num() < TargetCount && Pool.Num() > 0)
    {
        // Sum remaining weights
        float TotalWeight = 0.f;
        for (const FWeightedCandidate& C : Pool) { TotalWeight += C.Weight; }

        // Roll
        const float Roll = FMath::FRandRange(0.f, TotalWeight);

        float Accumulated = 0.f;
        int32 PickedIndex = 0;
        for (int32 i = 0; i < Pool.Num(); ++i)
        {
            Accumulated += Pool[i].Weight;
            if (Roll <= Accumulated)
            {
                PickedIndex = i;
                break;
            }
        }

        // Record the pick
        FNPCSpawnEntry Entry;
        Entry.RacerData       = Pool[PickedIndex].Racer;
        Entry.PatrolSplineName = Pool[PickedIndex].Racer->PatrolSplineName;
        Result.Add(Entry);

        // Remove from pool (no replacement)
        Pool.RemoveAtSwap(PickedIndex);
    }

    return Result;
}

// ---------------------------------------------------------------------------
// AreConditionsMet
// ---------------------------------------------------------------------------

bool UCourseNPCSpawnManager::AreConditionsMet(
    UNPCRacerData*       RacerData,
    URacingGameInstance* GI) const
{
    if (!RacerData) { return false; }

    // Empty conditions = always eligible
    if (RacerData->SpawnConditions.IsEmpty()) { return true; }

    for (const FNPCSpawnCondition& Cond : RacerData->SpawnConditions)
    {
        switch (Cond.ConditionType)
        {
        case ESpawnConditionType::Always:
            break; // always passes

        case ESpawnConditionType::MinCurrency:
        {
            if (!GI || !GI->GetVehicleInventory()) { return false; }
            if (GI->GetVehicleInventory()->PlayerCurrency < Cond.ThresholdValue)
            {
                return false;
            }
            break;
        }

        case ESpawnConditionType::MinOwnedVehicles:
        {
            if (!GI || !GI->GetVehicleInventory()) { return false; }
            if (GI->GetVehicleInventory()->OwnedVehicles.Num() < Cond.ThresholdValue)
            {
                return false;
            }
            break;
        }

        case ESpawnConditionType::StoryFlag:
        {
            if (!GI || !GI->GetPerkManager()) { return false; }
            if (!GI->GetPerkManager()->AchievedStoryFlags.Contains(Cond.StoryFlagName))
            {
                return false;
            }
            break;
        }

        default:
            break;
        }
    }

    return true;
}
