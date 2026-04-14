// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RacingVehicleTypes.h"
#include "VehiclePartData.generated.h"

/**
 * UVehiclePartData
 *
 * A Data Asset that defines all upgrade levels for a single part slot on
 * a specific vehicle (or family of vehicles).
 *
 * How to use in the editor:
 *   1. Right-click Content Browser ? Miscellaneous ? Data Asset ? VehiclePartData.
 *   2. Set PartSlot to the correct slot (e.g. Exhaust).
 *   3. Add entries to Levels — index 0 is "Stock / Level 1", index 1 is "Level 2", etc.
 *   4. Reference this asset from the vehicle's UVehicleDefinition AvailableParts map.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UVehiclePartData : public UDataAsset
{
    GENERATED_BODY()

public:

    /** Which slot this part occupies on the vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
    EPartSlot PartSlot = EPartSlot::PowerUnit;

    /**
     * Ordered list of upgrade levels, starting from the stock/base level at index 0.
     * The player always starts at level 0 when they first acquire the vehicle.
     * Each subsequent entry is one purchaseable upgrade tier.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part")
    TArray<FPartLevelData> Levels;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** Returns true if Level is a valid index into Levels. */
    UFUNCTION(BlueprintCallable, Category = "VehiclePart")
    bool IsValidLevel(int32 Level) const { return Levels.IsValidIndex(Level); }

    /** Returns the number of upgrade tiers defined (including stock). */
    UFUNCTION(BlueprintCallable, Category = "VehiclePart")
    int32 GetMaxLevel() const { return Levels.Num() - 1; }

    /**
     * Returns a pointer to the level data for the given level, or nullptr if
     * out of range.  Use IsValidLevel first if you need to guard the call site.
     */
    const FPartLevelData* GetLevelData(int32 Level) const
    {
        return Levels.IsValidIndex(Level) ? &Levels[Level] : nullptr;
    }
};
