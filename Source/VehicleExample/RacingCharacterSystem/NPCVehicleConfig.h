// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RacingVehicleTypes.h"
#include "VehicleDefinition.h"
#include "NPCVehicleConfig.generated.h"

/**
 * FNPCVehicleConfig
 *
 * A fully self-contained, designer-authored snapshot of one vehicle loadout
 * for an NPC racer.  Mirrors every saveable field on UOwnedVehicle so that
 * the NPC's car can be configured with the same granularity as the player's.
 *
 * Embedded directly in UNPCRacerData — no separate asset needed.
 *
 * At runtime, call UNPCRacerData::BuildOwnedVehicle() to produce a transient
 * UOwnedVehicle from this config (used for battle stat resolution and spawning).
 */
USTRUCT(BlueprintType)
struct FNPCVehicleConfig
{
    GENERATED_BODY()

    // -----------------------------------------------------------------------
    // Vehicle Model
    // -----------------------------------------------------------------------

    /**
     * The vehicle definition this NPC drives.
     * Must be set; everything else in this struct is meaningless without it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TSoftObjectPtr<UVehicleDefinition> VehicleDefinition;

    // -----------------------------------------------------------------------
    // Installed Parts
    // -----------------------------------------------------------------------

    /**
     * Maps each part slot to the installed level for this NPC's car.
     * Leave a slot out entirely to use stock (level 0).
     * No perk gates apply — designers set whatever level they want directly.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parts")
    TMap<EPartSlot, int32> InstalledPartLevels;

    // -----------------------------------------------------------------------
    // Gear Ratios
    // -----------------------------------------------------------------------

    /**
     * Per-gear ratio overrides.  Leave empty to use the definition's defaults.
     * If provided, the array should have one entry per forward gear as determined
     * by the installed Transmission part level (or the definition's default count).
     * Values are clamped to the definition's per-gear FGearRatioSpec at runtime.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
    TArray<float> GearRatioOverrides;

    // -----------------------------------------------------------------------
    // Tuning State
    // All fields default to the same values used by OwnedVehicle's ResetTuningToDefaults.
    // Designers only need to change values that differ from stock.
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Alignment")
    FAlignmentTuningState AlignmentTuning;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Brakes")
    FBrakeTuningState BrakeTuning;

    /** Rear differential LSD tuning (RWD and AWD). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|LSD")
    FLSDTuningState RearLSDTuning;

    /**
     * Front differential LSD tuning.
     * Only applied when the vehicle definition is AWD.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|LSD")
    FLSDTuningState FrontLSDTuning;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Suspension")
    FSuspensionTuningState SuspensionTuning;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Stabilizer")
    FStabilizerTuningState StabilizerTuning;

    /**
     * Torque balance front bias (AWD only, whole-number percentage 0–100).
     * Rear torque = 100 - FrontBias.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|TorqueBalance")
    FTorqueBalanceState TorqueBalance;
};
