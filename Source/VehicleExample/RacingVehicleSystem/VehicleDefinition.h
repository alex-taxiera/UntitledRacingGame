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
