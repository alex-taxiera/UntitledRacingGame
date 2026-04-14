// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "RacingVehicleTypes.h"
#include "VehiclePartData.h"
#include "VehicleDefinition.generated.h"

/**
 * UVehicleDefinition
 *
 * A Data Asset that fully describes one purchaseable vehicle model.
 * One asset = one model (e.g. "Nissan Skyline R34 GT-R").
 * Multiple player-owned instances all share the same definition asset.
 *
 * How to use in the editor:
 *   1. Right-click Content Browser ? Miscellaneous ? Data Asset ? VehicleDefinition.
 *   2. Fill in display metadata, purchase price, base stats, and engine definition.
 *   3. Set PawnClass to the Blueprint (or C++) pawn that represents this car.
 *   4. For each part slot the car supports, add an entry to AvailableParts
 *      pointing to the corresponding UVehiclePartData asset.
 *   5. Populate DefaultGearRatios — one FGearRatioSpec per forward gear.
 *      The stock transmission part level will reference the count here, but
 *      the Transmission part's GearCount can unlock additional entries.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UVehicleDefinition : public UDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Display / Shop
    // -----------------------------------------------------------------------

    /** Internal identifier (used for save data — do not change after shipping) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName VehicleID;

    /** Localised name shown in the shop and garage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /** Short flavour description */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText Description;

    /** Thumbnail used in shop and garage UI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    TSoftObjectPtr<UTexture2D> Thumbnail;

    /** Purchase price in in-game currency */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 PurchasePrice = 50000;

    // -----------------------------------------------------------------------
    // Pawn
    // -----------------------------------------------------------------------

    /**
     * The pawn class to spawn when this vehicle is selected for a race.
     * Should be a Blueprint child of AVehicleExamplePawn (or a subclass).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pawn")
    TSoftClassPtr<APawn> PawnClass;

    /**
     * The skeletal mesh asset for this vehicle.
     * The pawn Blueprint can also set this directly; this field lets the
     * inventory system display the mesh in menus without loading the pawn.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pawn")
    TSoftObjectPtr<USkeletalMesh> PreviewMesh;

    // -----------------------------------------------------------------------
    // Base Stats & Engine
    // -----------------------------------------------------------------------

    /** Chassis characteristics before any parts are applied */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    FVehicleBaseStats BaseStats;

    /** Engine characteristics before any tuning parts are applied */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    FEngineDefinition EngineDefinition;

    /** Drivetrain layout — controls which tuning sections (e.g. front LSD, torque balance) are available. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drivetrain")
    EDrivetrainType DrivetrainType = EDrivetrainType::RWD;

    // -----------------------------------------------------------------------
    // Parts
    // -----------------------------------------------------------------------

    /**
     * Maps each supported part slot to the upgrade ladder available for this
     * vehicle.  Slots not present in this map are not upgradeable on this car.
     *
     * Key   = EPartSlot
     * Value = UVehiclePartData asset that defines the levels for that slot
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parts")
    TMap<EPartSlot, TObjectPtr<UVehiclePartData>> AvailableParts;

    // -----------------------------------------------------------------------
    // Gear Ratios
    // -----------------------------------------------------------------------

    /**
     * One entry per forward gear (stock gear count).
     * Each entry carries the default ratio and the min/max the player can
     * tune to.  If the installed Transmission part unlocks more gears, extra
     * entries beyond this array's length use the last entry's spec as a
     * fallback — designers should extend this array to cover the max gear
     * count the highest transmission tier provides.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
    TArray<FGearRatioSpec> DefaultGearRatios;

    // -----------------------------------------------------------------------
    // Tuning Definitions
    // All ranges and unlock gates are set here by designers.
    // -----------------------------------------------------------------------

    /**
     * Wheel alignment tuning ranges (camber, toe, ride height, offset, tire width).
     * Alignment/ride-height/offset unlock via Suspension level; tire width via Tire level.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Alignment")
    FAlignmentTuningDef AlignmentTuning;

    /** Brake tuning ranges (ABS toggle, brake balance). Unlocks via Brake level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Brakes")
    FBrakeTuningDef BrakeTuning;

    /**
     * Rear differential LSD tuning.
     * Available on RWD and AWD vehicles (always populated when LSD slot is present).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|LSD")
    FLSDTuningDef RearLSDTuning;

    /**
     * Front differential LSD tuning.
     * Only relevant for AWD vehicles — ignored on FWD/RWD.
     * Leave defaults if not applicable; the UI gates visibility on DrivetrainType.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|LSD")
    FLSDTuningDef FrontLSDTuning;

    /** Suspension tuning ranges (spring rate, damper, damper balance). Unlocks via Suspension level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Suspension")
    FSuspensionTuningDef SuspensionTuning;

    /**
     * Stabilizer (anti-roll bar) tuning ranges.
     * Displayed separately in UI but shares the same Suspension-level unlock gate
     * as SuspensionTuning (uses SuspensionTuning.MinSuspensionLevelForTuning).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|Stabilizer")
    FStabilizerTuningDef StabilizerTuning;

    /**
     * Front-to-rear torque balance tuning (AWD only).
     * The UI should only expose this when DrivetrainType == AWD.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning|TorqueBalance")
    FTorqueBalanceDef TorqueBalance;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** Returns true if this vehicle has an upgradeable entry for the given slot. */
    UFUNCTION(BlueprintCallable, Category = "VehicleDefinition")
    bool SupportsPartSlot(EPartSlot Slot) const { return AvailableParts.Contains(Slot); }

    /** Returns the UVehiclePartData for the given slot, or nullptr if unsupported. */
    UFUNCTION(BlueprintCallable, Category = "VehicleDefinition")
    UVehiclePartData* GetPartData(EPartSlot Slot) const
    {
        const TObjectPtr<UVehiclePartData>* Found = AvailableParts.Find(Slot);
        return Found ? Found->Get() : nullptr;
    }

    /** Returns true when this vehicle is AWD and therefore exposes front LSD and torque balance tuning. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VehicleDefinition")
    bool IsAWD() const { return DrivetrainType == EDrivetrainType::AWD; }

    /**
     * Returns the GearRatioSpec for the given zero-based gear index.
     * If the index exceeds the array, returns the last defined spec as a
     * fallback so additional gears from high-tier transmissions still have
     * valid limits.
     */
    UFUNCTION(BlueprintCallable, Category = "VehicleDefinition")
    FGearRatioSpec GetGearRatioSpec(int32 GearIndex) const
    {
        if (DefaultGearRatios.IsEmpty()) { return FGearRatioSpec(); }
        const int32 ClampedIndex = FMath::Clamp(GearIndex, 0, DefaultGearRatios.Num() - 1);
        return DefaultGearRatios[ClampedIndex];
    }
};
