// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PerkTypes.h"
#include "RacingVehicleTypes.h"
#include "PerkData.generated.h"

// Forward declare to avoid circular includes; VehicleDefinition.h included in .cpp
class UVehicleDefinition;
class UPerkSkillEffect;

// ---------------------------------------------------------------------------
// Tree-specific payload structs
// Each is used only when PerkData.Tree matches the corresponding EPerkTree.
// Designers fill in only the relevant payload; others are ignored.
// ---------------------------------------------------------------------------

/**
 * Payload for EPerkTree::Vehicle perks.
 * Unlocking this perk makes the referenced vehicle appear in the shop.
 */
USTRUCT(BlueprintType)
struct FVehiclePerkPayload
{
    GENERATED_BODY()

    /**
     * The vehicle definition that becomes purchaseable when this perk is unlocked.
     * Set this to the UVehicleDefinition Data Asset for the car.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TSoftObjectPtr<UVehicleDefinition> VehicleDefinition;
};

/**
 * Payload for EPerkTree::Tuning perks.
 * Unlocking this perk allows the player to purchase the given part level
 * on any vehicle that supports that slot.
 */
USTRUCT(BlueprintType)
struct FTuningPerkPayload
{
    GENERATED_BODY()

    /** The part slot this perk unlocks purchasing rights for. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning")
    EPartSlot PartSlot = EPartSlot::PowerUnit;

    /**
     * The part level index this perk unlocks.
     * e.g. 1 = Level 2 (index 1), since 0 = stock and is always available.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning",
        meta = (ClampMin = "1"))
    int32 PartLevel = 1;
};

/**
 * Payload for EPerkTree::Driver stat-type perks.
 * Each stat perk raises one stat by Magnitude and contributes +1 to character level.
 */
USTRUCT(BlueprintType)
struct FDriverStatPerkPayload
{
    GENERATED_BODY()

    /** Which stat this perk raises. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DriverStat")
    EDriverStat Stat = EDriverStat::Attack;

    /** How much this perk adds to the stat. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DriverStat",
        meta = (ClampMin = "1"))
    int32 Magnitude = 1;
};

/**
 * Payload for EPerkTree::Driver skill-type perks.
 * A skill perk can carry any number of FPerkSkillEffect entries (the known C++ path)
 * and optionally a Blueprint UPerkSkillEffect subclass for custom logic.
 */
USTRUCT(BlueprintType)
struct FDriverSkillPerkPayload
{
    GENERATED_BODY()

    /**
     * One or more effects this skill contributes.
     * Passive entries are always active; active entries are applied at race start.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DriverSkill")
    TArray<FPerkSkillEffect> Effects;

    /**
     * Optional Blueprint-implemented effect object for logic that cannot be
     * expressed with the known ESkillEffectType values.
     * Leave null if all effects are covered by the Effects array.
     * Must be a subclass of UPerkSkillEffect.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DriverSkill")
    TSoftClassPtr<UPerkSkillEffect> CustomEffectClass;
};

/**
 * Payload for EPerkTree::Perks (permanent global effects).
 * Works identically to driver skills but lives in the Perks tree and
 * tends to target economy/meta effects rather than combat stats.
 */
USTRUCT(BlueprintType)
struct FGlobalPerkPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlobalPerk")
    TArray<FPerkSkillEffect> Effects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GlobalPerk")
    TSoftClassPtr<UPerkSkillEffect> CustomEffectClass;
};

// ---------------------------------------------------------------------------
// UPerkData
// ---------------------------------------------------------------------------

/**
 * UPerkData
 *
 * A Data Asset representing one node in any of the four skill trees.
 * Designers create one asset per perk, set its tree, fill in the matching
 * payload struct, and wire up prerequisites.
 *
 * How to use:
 *   1. Right-click Content Browser ? Miscellaneous ? Data Asset ? PerkData.
 *   2. Set PerkID to a unique FName (never change after shipping; used in save data).
 *   3. Set Tree to the correct tree.
 *   4. Fill in ONLY the payload struct that matches the tree:
 *        Vehicle  ? VehiclePayload
 *        Tuning   ? TuningPayload
 *        Driver (stat)  ? StatPayload   (leave SkillPayload empty)
 *        Driver (skill) ? SkillPayload  (leave StatPayload empty)
 *        Perks    ? GlobalPerkPayload
 *   5. Add prerequisite perk IDs and any story flags required.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UPerkData : public UDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------

    /**
     * Unique stable identifier. Used in save data and cross-perk references.
     * NEVER rename this after the game ships.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName PerkID;

    /** Display name shown in the skill tree UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /** Flavour description shown on hover / selection. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText Description;

    /** Which tree this perk lives in. Controls which payload struct is used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tree")
    EPerkTree Tree = EPerkTree::Driver;

    /** Number of skill points required to unlock this perk. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy",
        meta = (ClampMin = "1"))
    int32 PointCost = 1;

    // -----------------------------------------------------------------------
    // Prerequisites
    // -----------------------------------------------------------------------

    /**
     * ALL of these perk IDs must already be unlocked before this perk
     * can be purchased. Leave empty for root nodes.
     * Designers can require 1, 2, or more prerequisite perks freely.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prerequisites")
    TArray<FName> PrerequisitePerkIDs;

    /**
     * ALL of these story flags must be present in the player's achieved set
     * before this perk is available. Leave empty for no story gate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prerequisites")
    TArray<FStoryProgressionRequirement> StoryRequirements;

    // -----------------------------------------------------------------------
    // Tree Payloads  (fill in only the one matching your Tree value)
    // -----------------------------------------------------------------------

    /** Filled when Tree == Vehicle. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload|Vehicle",
        meta = (EditCondition = "Tree == EPerkTree::Vehicle"))
    FVehiclePerkPayload VehiclePayload;

    /** Filled when Tree == Tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload|Tuning",
        meta = (EditCondition = "Tree == EPerkTree::Tuning"))
    FTuningPerkPayload TuningPayload;

    /**
     * Filled when Tree == Driver and this perk raises a combat stat.
     * A perk is treated as a stat perk when StatPayload.Magnitude > 0.
     * Stat perks each contribute +1 to the character's level.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload|Driver",
        meta = (EditCondition = "Tree == EPerkTree::Driver"))
    FDriverStatPerkPayload StatPayload;

    /**
     * Filled when Tree == Driver and this perk grants a skill effect.
     * A perk can have both stat AND skill payloads if desired.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload|Driver",
        meta = (EditCondition = "Tree == EPerkTree::Driver"))
    FDriverSkillPerkPayload SkillPayload;

    /** Filled when Tree == Perks. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload|Perks",
        meta = (EditCondition = "Tree == EPerkTree::Perks"))
    FGlobalPerkPayload GlobalPerkPayload;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * Returns true if this perk contributes to the character's level.
     * Only Driver-tree stat perks count (StatPayload.Magnitude > 0).
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PerkData")
    bool IsStatPerk() const
    {
        return Tree == EPerkTree::Driver && StatPayload.Magnitude > 0;
    }

    /**
     * Returns all FPerkSkillEffect entries this perk contributes.
     * Aggregates from both SkillPayload.Effects and GlobalPerkPayload.Effects.
     */
    UFUNCTION(BlueprintCallable, Category = "PerkData")
    TArray<FPerkSkillEffect> GetAllEffects() const;

    /**
     * Returns the soft class pointer to the custom Blueprint effect, if any.
     * Checks SkillPayload first, then GlobalPerkPayload.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PerkData")
    TSoftClassPtr<UPerkSkillEffect> GetCustomEffectClass() const;
};
