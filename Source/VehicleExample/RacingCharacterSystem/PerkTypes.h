// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PerkTypes.generated.h"

// Forward declaration — PerkSkillEffect.h is not included here to avoid circular deps.
class UPerkSkillEffect;

// ---------------------------------------------------------------------------
// Perk Trees
// ---------------------------------------------------------------------------

/** The four skill trees a perk can belong to. */
UENUM(BlueprintType)
enum class EPerkTree : uint8
{
    Vehicle     UMETA(DisplayName = "Vehicle"),
    Tuning      UMETA(DisplayName = "Tuning"),
    Driver      UMETA(DisplayName = "Driver"),
    Perks       UMETA(DisplayName = "Perks"),
};

// ---------------------------------------------------------------------------
// Driver Stats
// ---------------------------------------------------------------------------

/** The four combat stats every driver (player and NPC) possesses. */
UENUM(BlueprintType)
enum class EDriverStat : uint8
{
    Attack      UMETA(DisplayName = "Attack"),
    Defense     UMETA(DisplayName = "Defense"),
    Health      UMETA(DisplayName = "Health"),
    Toughness   UMETA(DisplayName = "Toughness"),
};

// ---------------------------------------------------------------------------
// Skill Effect Types
// ---------------------------------------------------------------------------

/**
 * Known C++ effect types.
 * The battle system reads these to apply effects without any Blueprint overhead.
 * Add new entries here as new effect types are designed.
 * For effects that don't fit any known type, use Custom and provide a
 * UPerkSkillEffect Blueprint subclass on the perk.
 */
UENUM(BlueprintType)
enum class ESkillEffectType : uint8
{
    /** Reduces damage taken from being behind an opponent (fraction, e.g. 0.1 = -10%) */
    DamageReduction             UMETA(DisplayName = "Damage Reduction"),

    /** Flat bonus added to Attack stat */
    AttackBoost                 UMETA(DisplayName = "Attack Boost"),

    /** Flat bonus added to Defense stat */
    DefenseBoost                UMETA(DisplayName = "Defense Boost"),

    /** Flat bonus added to Health stat */
    HealthBoost                 UMETA(DisplayName = "Health Boost"),

    /** Flat bonus added to Toughness stat */
    ToughnessBoost              UMETA(DisplayName = "Toughness Boost"),

    /** Reduces damage taken from colliding with walls/barriers (fraction) */
    WallDamageReduction         UMETA(DisplayName = "Wall Damage Reduction"),

    /**
     * Temporary Attack bonus applied at race start.
     * Duration (seconds) stored in FPerkEffectData::Duration.
     */
    AttackBoostTimed            UMETA(DisplayName = "Attack Boost (Timed)"),

    /**
     * Temporary Defense bonus applied at race start.
     * Duration (seconds) stored in FPerkEffectData::Duration.
     */
    DefenseBoostTimed           UMETA(DisplayName = "Defense Boost (Timed)"),

    /** Restores health whenever the player activates nitro. Magnitude = HP restored per use. */
    HealthOnNitro               UMETA(DisplayName = "Health on Nitro"),

    /** Multiplies post-race money reward. Magnitude = additive fraction (e.g. 0.2 = +20%) */
    MoneyGainBoost              UMETA(DisplayName = "Money Gain Boost"),

    /** Increases the player's maximum currency cap. Magnitude = flat amount added. */
    WalletSizeIncrease          UMETA(DisplayName = "Wallet Size Increase"),

    /**
     * Increases the number of skill slots available to the driver.
     * Magnitude = whole number of additional slots granted (e.g. 1.0 = +1 slot).
     * Must be a passive effect (bIsPassive = true).
     */
    SkillSlotIncrease           UMETA(DisplayName = "Skill Slot Increase"),

    /**
     * Effect logic is implemented entirely in a Blueprint UPerkSkillEffect subclass.
     * The C++ battle system will invoke OnEffectApplied / OnEffectRemoved on the object.
     */
    Custom                      UMETA(DisplayName = "Custom (Blueprint)"),
};

// ---------------------------------------------------------------------------
// Skill Effect Data
// ---------------------------------------------------------------------------

/**
 * Describes one discrete effect contributed by a perk skill.
 * Passive effects (bIsPassive = true) are always active while the perk is unlocked.
 * Active effects are applied at the start of a race battle and removed after Duration.
 */
USTRUCT(BlueprintType)
struct FPerkEffectData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    ESkillEffectType EffectType = ESkillEffectType::AttackBoost;

    /**
     * Magnitude of the effect.
     * Interpretation depends on EffectType — see enum comments above.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    float Magnitude = 0.0f;

    /**
     * Duration in seconds for timed effects.
     * Ignored for passive effects (bIsPassive = true).
     * 0 means the effect lasts the entire race.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect",
        meta = (EditCondition = "!bIsPassive"))
    float Duration = 0.0f;

    /**
     * If true, this effect is always active while the perk is unlocked.
     * If false, it is applied at race start and removed after Duration.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bIsPassive = true;
};

// ---------------------------------------------------------------------------
// Story Progression Requirement
// ---------------------------------------------------------------------------

/**
 * A named flag that must be present in the player's achieved story flags
 * before a perk is available for purchase.
 *
 * Designers use descriptive names like "chapter2.complete" or "rival.bob.defeated".
 * The story system sets these flags; the perk manager checks them.
 */
USTRUCT(BlueprintType)
struct FStoryProgressionRequirement
{
    GENERATED_BODY()

    /**
     * The flag identifier.  Must match exactly what the story system will set.
     * Convention: use dot-separated lowercase, e.g. "story.act1.complete".
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story")
    FName Flag;
};

// ---------------------------------------------------------------------------
// Driver Stat Block
// ---------------------------------------------------------------------------

/** The resolved combat stats for one driver after all perk contributions. */
USTRUCT(BlueprintType)
struct FDriverStatBlock
{
    GENERATED_BODY()

    /** Base attack before any perk additions; perk magnitudes are added on top. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Attack = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Defense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Health = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Toughness = 0;

    /**
     * How many skill perks can be equipped simultaneously.
     * This is the base value before any SkillSlotIncrease perk effects are applied.
     * Set by the game mode or difficulty context, not stored in the stat block directly —
     * this field reflects the resolved total after all slot-increase effects.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 SkillSlots = 3;
};

// ---------------------------------------------------------------------------
// Racer Battle State
// ---------------------------------------------------------------------------

/**
 * Runtime state for one racer during a battle.
 * Holds everything URaceBattleManager needs to track health, resolve damage,
 * manage live effects, and answer display queries.
 *
 * Not saved — rebuilt fresh at the start of each race.
 */
USTRUCT(BlueprintType)
struct FRacerBattleState
{
    GENERATED_BODY()

    /** Name shown in the battle UI for this racer. */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    FText DisplayName;

    /** Fully resolved combat stats for this racer (base + all perk bonuses). */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    FDriverStatBlock ResolvedStats;

    /** Maximum health for this battle, derived from ResolvedStats.Health. */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    float MaxHealth = 0.0f;

    /** Current health. Starts at MaxHealth; battle ends when this reaches 0. */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    float CurrentHealth = 0.0f;

    /**
     * Passive effects active for the entire race.
     * Populated from CharacterPerkState::GetAllPassiveEffects at battle start.
     * Does not include SkillSlotIncrease (handled by the perk manager).
     */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<FPerkEffectData> PassiveEffects;

    /**
     * Timed effects currently running (applied at race start, expire after Duration).
     * Entries are removed when their elapsed time exceeds Duration.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<FPerkEffectData> ActiveTimedEffects;

    /**
     * Elapsed time in seconds for each entry in ActiveTimedEffects (parallel array).
     * Incremented every Tick; entry is removed when elapsed >= effect Duration.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<float> TimedEffectElapsed;

    /**
     * Instantiated Blueprint custom effect objects for this racer.
     * Created at battle start from CharacterPerkState::GetAllCustomEffectClasses.
     * ApplyEffect has already been called on each one.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<TObjectPtr<UPerkSkillEffect>> LiveCustomEffects;

    /**
     * The perk IDs of equipped skill perks, copied at battle start for display.
     * Used by URaceBattleManager::GetEquippedSkillNames.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    TArray<FName> EquippedSkillPerkIDs;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** Returns the sum of all passive DamageReduction magnitudes for this racer. */
    float GetTotalDamageReduction() const
    {
        float Total = 0.0f;
        for (const FPerkEffectData& E : PassiveEffects)
        {
            if (E.bIsPassive && E.EffectType == ESkillEffectType::DamageReduction)
            {
                Total += E.Magnitude;
            }
        }
        // Also check timed effects still running
        for (const FPerkEffectData& E : ActiveTimedEffects)
        {
            if (E.EffectType == ESkillEffectType::DamageReduction)
            {
                Total += E.Magnitude;
            }
        }
        return Total;
    }

    /** Returns the sum of all passive/timed WallDamageReduction magnitudes. */
    float GetTotalWallDamageReduction() const
    {
        float Total = 0.0f;
        for (const FPerkEffectData& E : PassiveEffects)
        {
            if (E.bIsPassive && E.EffectType == ESkillEffectType::WallDamageReduction)
            {
                Total += E.Magnitude;
            }
        }
        for (const FPerkEffectData& E : ActiveTimedEffects)
        {
            if (E.EffectType == ESkillEffectType::WallDamageReduction)
            {
                Total += E.Magnitude;
            }
        }
        return Total;
    }

    /** Returns the total HealthOnNitro heal amount across all active effects. */
    float GetTotalHealthOnNitro() const
    {
        float Total = 0.0f;
        for (const FPerkEffectData& E : PassiveEffects)
        {
            if (E.bIsPassive && E.EffectType == ESkillEffectType::HealthOnNitro)
            {
                Total += E.Magnitude;
            }
        }
        return Total;
    }
};
