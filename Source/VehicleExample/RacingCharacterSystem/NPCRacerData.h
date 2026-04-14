// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "PerkTypes.h"
#include "PerkData.h"
#include "CharacterPerkState.h"
#include "NPCVehicleConfig.h"
#include "NPCRacerData.generated.h"

class UOwnedVehicle;

/**
 * UNPCRacerData
 *
 * A Data Asset that fully defines one NPC racer the player can face.
 * Everything is static and designer-set — NPCs have no progression, no
 * currency, and no save state.
 *
 * How to use in the editor:
 *   1. Right-click Content Browser ? Miscellaneous ? Data Asset ? NPCRacerData.
 *   2. Set RacerName and optionally a Portrait and Description.
 *   3. Set BaseStats — the raw combat stats before any perk contributions.
 *   4. Add perk IDs to PerkIDs.  These must match the PerkID field on
 *      UPerkData assets in your project.  No prerequisites are checked for
 *      NPCs — whatever is listed here is treated as unlocked.
 *   5. Fill in VehicleConfig with the vehicle definition and any part/tuning
 *      overrides.  Stock values need not be changed.
 *
 * At runtime, call the helper functions to get resolved stats, a perk state
 * object, or a transient UOwnedVehicle for spawning and battle logic.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UNPCRacerData : public UDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Identity & Display
    // -----------------------------------------------------------------------

    /** The name shown to the player when this NPC is encountered. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText RacerName;

    /** Optional short bio or battle quote. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Description;

    /** Portrait image used in battle UI and NPC roster screens. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    TSoftObjectPtr<UTexture2D> Portrait;

    // -----------------------------------------------------------------------
    // Combat Stats
    // -----------------------------------------------------------------------

    /**
     * The NPC's base combat stats before any perk contributions are added.
     * Perk stat payloads from PerkIDs are summed on top of these values.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    FDriverStatBlock BaseStats;

    // -----------------------------------------------------------------------
    // Perk Loadout
    // -----------------------------------------------------------------------

    /**
     * The set of perks this NPC has.  Reference perks by their PerkID FName.
     * All listed perks are treated as active — no prerequisite or point checks.
     * Only Driver and Perks tree entries have any mechanical effect on NPCs;
     * Vehicle and Tuning tree perks are ignored (NPCs don't use the shop).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perks")
    TArray<FName> PerkIDs;

    // -----------------------------------------------------------------------
    // Vehicle Configuration
    // -----------------------------------------------------------------------

    /**
     * The NPC's vehicle loadout.  Set the vehicle definition and override
     * any part levels and tuning values you want to differ from stock.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
    FNPCVehicleConfig VehicleConfig;

    // -----------------------------------------------------------------------
    // Runtime helpers
    // -----------------------------------------------------------------------

    /**
     * Builds and returns a transient UCharacterPerkState populated with this
     * NPC's PerkIDs.  The Outer should be the object that will own the state
     * for the duration of the race (e.g. a battle manager or NPC controller).
     *
     * The returned object is not saved and not registered with any inventory.
     */
    UFUNCTION(BlueprintCallable, Category = "NPCRacerData")
    UCharacterPerkState* CreatePerkState(UObject* Outer) const;

    /**
     * Returns the fully-resolved FDriverStatBlock for this NPC by applying
     * all Driver-tree stat perk contributions from PerkIDs on top of BaseStats.
     *
     * @param AllPerks  The full perk catalogue (from UPlayerPerkManager::AllPerks
     *                  or any other source that has loaded the perk assets).
     */
    UFUNCTION(BlueprintCallable, Category = "NPCRacerData")
    FDriverStatBlock ComputeStats(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns this NPC's character level: the count of Driver-tree stat perks
     * in PerkIDs.  Used for display and difficulty gauging only.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPCRacerData")
    int32 ComputeLevel(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Constructs a transient UOwnedVehicle from VehicleConfig.
     *
     * The returned vehicle is NOT added to any inventory and NOT saved.
     * It is intended for battle stat resolution and pawn spawning only.
     * Gear ratios are taken from GearRatioOverrides if provided, otherwise
     * fall back to the definition's defaults.  All tuning state is copied
     * directly from VehicleConfig without unlock-gate checks.
     *
     * Returns nullptr if VehicleConfig.VehicleDefinition is null or not loaded.
     *
     * @param Outer  The owning object (e.g. a battle manager).
     */
    UFUNCTION(BlueprintCallable, Category = "NPCRacerData")
    UOwnedVehicle* BuildOwnedVehicle(UObject* Outer) const;
};
