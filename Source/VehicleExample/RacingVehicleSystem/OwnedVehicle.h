// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RacingVehicleTypes.h"
#include "VehicleDefinition.h"
#include "OwnedVehicle.generated.h"

/**
 * UOwnedVehicle
 *
 * A runtime object representing one player-owned vehicle instance.
 * Two instances of the same definition (e.g. two R34s) are two separate
 * UOwnedVehicle objects with independent part levels and gear ratios.
 *
 * Owned by UVehicleInventory and lives for the lifetime of the game session.
 * The fields marked SaveGame are the only ones that need to be persisted.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UOwnedVehicle : public UObject
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------

    /**
     * Unique ID for this owned instance.
     * Generated at purchase time and used as the key in save data.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Identity")
    FGuid InstanceID;

    /**
     * Optional player-assigned nickname for this car (e.g. "Track R34").
     * Empty string = use the definition's DisplayName.
     */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString Nickname;

    /** The vehicle model this instance is based on */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Definition")
    TObjectPtr<UVehicleDefinition> Definition;

    // -----------------------------------------------------------------------
    // Installed Parts  (key = EPartSlot, value = installed level index)
    // -----------------------------------------------------------------------

    /**
     * Maps each slot to the currently installed part level (0 = stock).
     * Slots absent from this map are treated as level 0.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Parts")
    TMap<EPartSlot, int32> InstalledPartLevels;

    // -----------------------------------------------------------------------
    // Gear Ratios  (player-tuned, per forward gear)
    // -----------------------------------------------------------------------

    /**
     * Player-set gear ratios for each forward gear.
     * Populated with defaults from the Definition when the vehicle is purchased.
     * Count reflects the number of gears unlocked by the current Transmission
     * part level.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Transmission")
    TArray<float> TunedGearRatios;

    // -----------------------------------------------------------------------
    // Nitro State  (runtime only — refilled during races)
    // -----------------------------------------------------------------------

    /** Current nitro fuel remaining (0.0 – NitroCapacity from effective stats) */
    UPROPERTY(BlueprintReadWrite, Category = "Nitro")
    float CurrentNitro = 0.0f;

    // -----------------------------------------------------------------------
    // Factory
    // -----------------------------------------------------------------------

    /**
     * Creates and initialises a new UOwnedVehicle for the given definition.
     * Sets stock part levels (0 for every supported slot), default gear ratios,
     * and a fresh GUID.  Call this when the player purchases a vehicle.
     *
     * @param Outer     The object that will own this instance (typically UVehicleInventory).
     * @param InDefinition  The vehicle model being purchased.
     */
    static UOwnedVehicle* CreateFromDefinition(UObject* Outer, UVehicleDefinition* InDefinition);

    // -----------------------------------------------------------------------
    // Part Queries & Mutations
    // -----------------------------------------------------------------------

    /**
     * Returns the currently installed level for the given slot.
     * Returns 0 (stock) if the slot has no entry yet.
     */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    int32 GetInstalledLevel(EPartSlot Slot) const;

    /**
     * Installs a new part level for the given slot.
     * Does NOT validate points/price — that is the inventory's responsibility.
     * Automatically refreshes effective stats and (for Transmission) resizes
     * TunedGearRatios to the new gear count.
     *
     * @param Slot   The slot to upgrade.
     * @param Level  The new level index to install.
     * @return       True if the level was valid and was applied.
     */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    bool SetPartLevel(EPartSlot Slot, int32 Level);

    // -----------------------------------------------------------------------
    // Gear Ratio Tuning
    // -----------------------------------------------------------------------

    /**
     * Sets the player-tuned ratio for a specific gear.
     * The value is clamped to the [Min, Max] range from the definition's
     * GearRatioSpec for that gear index.
     *
     * @param GearIndex  Zero-based forward gear index.
     * @param Ratio      Desired ratio (will be clamped to spec).
     * @return           True if the gear index was valid and the value was applied.
     */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    bool SetGearRatio(int32 GearIndex, float Ratio);

    /** Resets all gear ratios to the definition defaults. */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    void ResetGearRatiosToDefault();

    // -----------------------------------------------------------------------
    // Effective Stats
    // -----------------------------------------------------------------------

    /**
     * Computes and returns the fully-resolved stats for this vehicle instance
     * by summing the base stats, all installed part modifiers, and the current
     * tuned gear ratios.
     *
     * This is recalculated on demand.  Call it once per relevant event
     * (part installed, gears changed, race start) rather than every tick.
     */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    FEffectiveVehicleStats ComputeEffectiveStats() const;

    // -----------------------------------------------------------------------
    // Display helpers
    // -----------------------------------------------------------------------

    /** Returns Nickname if set, otherwise the definition's DisplayName as a string. */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    FString GetDisplayName() const;

private:

    /** Resizes TunedGearRatios to match the gear count from the installed Transmission level. */
    void RefreshGearCount();
};
