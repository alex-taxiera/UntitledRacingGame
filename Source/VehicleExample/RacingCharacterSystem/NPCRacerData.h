// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "PerkTypes.h"
#include "PerkData.h"
#include "CharacterPerkState.h"
#include "NPCVehicleConfig.h"
#include "RacingAITypes.h"
#include "NPCRacerData.generated.h"

class UOwnedVehicle;

// ---------------------------------------------------------------------------
// Spawn condition
// ---------------------------------------------------------------------------

/**
 * ESpawnConditionType
 *
 * What kind of condition must be met for this NPC to be eligible to spawn.
 * Designed to be extensible — add new entries here and handle them in
 * UCourseNPCSpawnManager::EvaluateCondition().
 */
UENUM(BlueprintType)
enum class ESpawnConditionType : uint8
{
    /** Always eligible. Use this as a default/placeholder. */
    Always          UMETA(DisplayName = "Always"),

    /**
     * Eligible only when the player's total currency is at or above a threshold.
     * Use to gate wealthy/endgame rivals.
     */
    MinCurrency     UMETA(DisplayName = "Min Currency"),

    /**
     * Eligible only when the player has purchased at least N vehicles.
     * Use to require some progression before a rival appears.
     */
    MinOwnedVehicles UMETA(DisplayName = "Min Owned Vehicles"),

    /**
     * Eligible only when a named story flag has been set on the game instance.
     * Use for story-gated rivals (e.g. unlocked after beating a chapter boss).
     */
    StoryFlag        UMETA(DisplayName = "Story Flag Set"),
};

/**
 * FNPCSpawnCondition
 *
 * A single eligibility condition for an NPC to appear on the course.
 * All conditions in an NPC's SpawnConditions array must pass for the
 * NPC to be included in the spawn pool.
 */
USTRUCT(BlueprintType)
struct FNPCSpawnCondition
{
    GENERATED_BODY()

    /** What kind of check to perform. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnCondition")
    ESpawnConditionType ConditionType = ESpawnConditionType::Always;

    /**
     * Numeric threshold used by MinCurrency and MinOwnedVehicles.
     * Ignored for Always and StoryFlag.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnCondition",
        meta = (EditCondition = "ConditionType == ESpawnConditionType::MinCurrency || ConditionType == ESpawnConditionType::MinOwnedVehicles"))
    int32 ThresholdValue = 0;

    /**
     * Story flag name used by the StoryFlag condition.
     * Must match exactly what the story system sets on URacingGameInstance.
     * Ignored for other condition types.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnCondition",
        meta = (EditCondition = "ConditionType == ESpawnConditionType::StoryFlag"))
    FName StoryFlagName;
};

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

    /**
     * Which of the PerkIDs above are actively equipped in skill slots.
     * Only effects from these perks apply in battle.
     * Every entry here must also appear in PerkIDs.
     *
     * For NPCs there is no hard slot-count enforcement — the designer is
     * responsible for keeping this list at or below the intended slot count.
     * The count is informational: BaseSkillSlots + any SkillSlotIncrease perks
     * in PerkIDs determines what "should" fit.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perks")
    TArray<FName> EquippedSkillPerkIDs;

    /**
     * The baseline skill slot count for this NPC.
     * Used only for display / difficulty gauging; not enforced at runtime.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perks")
    int32 BaseSkillSlots = 3;

    // -----------------------------------------------------------------------
    // Course Spawning
    // -----------------------------------------------------------------------

    /**
     * Relative spawn weight for this NPC.
     * Higher values make the NPC more likely to be picked during the
     * weighted random selection in UCourseNPCSpawnManager.
     * Default 1.0 = equal chance.  Use 0.1 for rare rivals, 3.0 for common.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning",
        meta = (ClampMin = "0.01"))
    float SpawnWeight = 1.0f;

    /**
     * Name of the patrol spline this NPC should follow when idle.
     * Must match a key in ACourseSplineActor's Splines map.
     * Leave empty to assign any available spline.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
    FName PatrolSplineName;

    /**
     * All conditions in this array must pass for this NPC to be included
     * in the spawn pool.  An empty array is equivalent to Always-eligible.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
    TArray<FNPCSpawnCondition> SpawnConditions;

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
    // AI Configuration
    // -----------------------------------------------------------------------

    /**
     * Full AI behaviour configuration for this NPC racer.
     * Controls state machine behaviour, behavior rules, nitro usage,
     * cornering parameters, rubber-band, and per-NPC difficulty scaling.
     * The AI controller reads this at possession time.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    FRacingAIConfig AIConfig;

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
