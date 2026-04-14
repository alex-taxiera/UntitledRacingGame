// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PerkTypes.h"
#include "PerkData.h"
#include "CharacterPerkState.generated.h"

/**
 * UCharacterPerkState
 *
 * Stores the set of unlocked perks for one driver — either the player or an NPC.
 *
 * Player usage:
 *   - Lives on UPlayerPerkManager, which handles purchase validation.
 *   - UnlockedPerkIDs is SaveGame so it persists across sessions.
 *
 * NPC usage:
 *   - Created directly and populated by designers via UNPCDriverData assets.
 *   - Not saved; rebuilt from the asset at runtime.
 *
 * This class is deliberately free of any economy logic — it only knows
 * which perks are unlocked and what they compute to.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UCharacterPerkState : public UObject
{
    GENERATED_BODY()

public:

    /**
     * The set of perk IDs currently unlocked for this driver.
     * Marked SaveGame so the player's state survives session changes.
     * For NPCs this is populated from their designer-defined preset and not saved.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Perks")
    TArray<FName> UnlockedPerkIDs;

    // -----------------------------------------------------------------------
    // Unlock state
    // -----------------------------------------------------------------------

    /** Returns true if the perk with the given ID is in the unlocked set. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CharacterPerkState")
    bool HasPerk(FName PerkID) const;

    /**
     * Directly adds a perk ID to the unlocked set.
     * Does NOT validate prerequisites or cost — that is UPlayerPerkManager's job.
     * Use this for NPC setup or for the player manager after validation passes.
     */
    void AddPerk(FName PerkID);

    // -----------------------------------------------------------------------
    // Stat computation
    // -----------------------------------------------------------------------

    /**
     * Computes the resolved FDriverStatBlock by summing base stats and all
     * stat-perk contributions from UnlockedPerkIDs.
     *
     * @param AllPerks   The full catalogue of perk assets (from UPlayerPerkManager
     *                   or from the NPC data asset).
     * @param BaseStats  The character's base stats before any perk bonuses.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    FDriverStatBlock ComputeStats(const TArray<UPerkData*>& AllPerks,
                                  const FDriverStatBlock& BaseStats) const;

    /**
     * Returns the character's level: the total count of unlocked Driver-tree
     * stat perks.  Each stat perk (where IsStatPerk() == true) counts as +1.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CharacterPerkState")
    int32 ComputeLevel(const TArray<UPerkData*>& AllPerks) const;

    // -----------------------------------------------------------------------
    // Effect queries
    // -----------------------------------------------------------------------

    /**
     * Returns all passive FPerkSkillEffect entries across every unlocked perk.
     * Used by the battle system to build the permanent modifier list at race start.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    TArray<FPerkSkillEffect> GetAllPassiveEffects(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns all active (non-passive) FPerkSkillEffect entries across every
     * unlocked perk.  Applied at race start, removed after their Duration.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    TArray<FPerkSkillEffect> GetAllActiveEffects(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns the soft class pointers for any custom Blueprint effect classes
     * from unlocked perks.  The battle system instantiates these and calls
     * ApplyEffect on each one.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    TArray<TSoftClassPtr<UPerkSkillEffect>> GetAllCustomEffectClasses(
        const TArray<UPerkData*>& AllPerks) const;

    // -----------------------------------------------------------------------
    // Prerequisite check  (used by UPlayerPerkManager before a purchase)
    // -----------------------------------------------------------------------

    /**
     * Returns true if all prerequisites for the given perk are satisfied:
     *   - Every PrerequisitePerkID is already in UnlockedPerkIDs.
     *   - Every StoryRequirement flag is present in AchievedStoryFlags.
     */
    bool ArePrerequisitesMet(const UPerkData* Perk,
                              const TSet<FName>& AchievedStoryFlags) const;

private:

    /** Finds a perk by ID in AllPerks; returns nullptr if not found. */
    static UPerkData* FindPerk(const TArray<UPerkData*>& AllPerks, FName PerkID);
};
