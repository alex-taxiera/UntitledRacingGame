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
    // Tuning State  (all saved per instance, set from the garage tuning screen)
    // -----------------------------------------------------------------------

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FAlignmentTuningState AlignmentTuning;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FBrakeTuningState BrakeTuning;

    /** Rear differential LSD tuning (RWD and AWD). */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FLSDTuningState RearLSDTuning;

    /**
     * Front differential LSD tuning.
     * Only meaningful when Definition->IsAWD() is true.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FLSDTuningState FrontLSDTuning;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FSuspensionTuningState SuspensionTuning;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FStabilizerTuningState StabilizerTuning;

    /**
     * Torque balance (AWD only).
     * FrontBias is a whole-number percentage; rear = 100 - FrontBias.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Tuning")
    FTorqueBalanceState TorqueBalance;

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
    // Tuning Unlock Queries
    // -----------------------------------------------------------------------

    /** Returns true when alignment tuning (camber/toe/ride height/offset) is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsAlignmentTuningUnlocked() const;

    /** Returns true when tire-width tuning is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsTireWidthTuningUnlocked() const;

    /** Returns true when brake tuning (ABS, balance) is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsBrakeTuningUnlocked() const;

    /** Returns true when rear LSD tuning is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsRearLSDTuningUnlocked() const;

    /**
     * Returns true when front LSD tuning is unlocked.
     * Always false on non-AWD vehicles regardless of LSD level.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsFrontLSDTuningUnlocked() const;

    /** Returns true when suspension tuning (spring rate, damper) is unlocked. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsSuspensionTuningUnlocked() const;

    /**
     * Returns true when stabilizer tuning is unlocked.
     * Shares the same Suspension-level gate as IsSuspensionTuningUnlocked().
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsStabilizerTuningUnlocked() const;

    /**
     * Returns true when torque balance tuning is available.
     * Requires AWD drivetrain — no additional part level gate.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OwnedVehicle|Tuning")
    bool IsTorqueBalanceTuningUnlocked() const;

    // -----------------------------------------------------------------------
    // Tuning Setters  (each validates unlock state and clamps to definition spec)
    // Returns false if the tuning is not yet unlocked or the value is invalid.
    // -----------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetCamberFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetCamberRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetToeFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetToeRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetRideHeightFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetRideHeightRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetOffsetFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetOffsetRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetTireWidthFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetTireWidthRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetABSEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetBrakeBalance(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetRearLSDType(ELSDType Type);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetRearLSDInitialTorque(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetRearLSDRatio(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetFrontLSDType(ELSDType Type);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetFrontLSDInitialTorque(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetFrontLSDRatio(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetSpringRateFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetSpringRateRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetDamperFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetDamperRear(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetDamperBalance(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetStabilizerFront(float Value);

    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetStabilizerRear(float Value);

    /**
     * Sets the front torque bias as a whole-number percentage (0–100).
     * Rear torque = 100 - FrontBias.  Fails if vehicle is not AWD.
     */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle|Tuning")
    bool SetTorqueBalanceFrontBias(int32 FrontBias);

    /** Resets all tuning fields to definition defaults. */
    UFUNCTION(BlueprintCallable, Category = "OwnedVehicle")
    void ResetTuningToDefaults();

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
