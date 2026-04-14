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

    /**
     * The currently equipped skill perks, in slot order.
     * Only effects from these perks are applied in battle — unlocking a skill
     * perk does not automatically activate it.
     *
     * Length is capped at GetSkillSlotCount() at the time of equipping.
     * Saved for the player; for NPCs this is set directly from UNPCRacerData.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Perks")
    TArray<FName> EquippedSkillPerkIDs;

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
    // Skill Slots & Equipping
    // -----------------------------------------------------------------------

    /**
     * Returns the total number of skill slots available to this driver.
     * = BaseSlots + sum of all SkillSlotIncrease passive effects from UNLOCKED perks
     *   (slot-increase perks always apply regardless of the equipped set, since
     *    equipping them would use up a slot they are meant to provide).
     *
     * @param AllPerks   Full perk catalogue.
     * @param BaseSlots  The starting slot count from the game mode / difficulty.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CharacterPerkState")
    int32 GetSkillSlotCount(const TArray<UPerkData*>& AllPerks, int32 BaseSlots) const;

    /** Returns true if the given perk ID is currently in the equipped set. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CharacterPerkState")
    bool IsSkillEquipped(FName PerkID) const;

    /**
     * Returns true if the skill can be equipped:
     *   - The perk is unlocked.
     *   - The perk is a Driver-tree skill perk (has effects or a custom class).
     *   - It is not already equipped.
     *   - There is at least one free slot.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CharacterPerkState")
    bool CanEquipSkill(FName PerkID, const TArray<UPerkData*>& AllPerks, int32 BaseSlots) const;

    /**
     * Equips the skill into the next available slot.
     * Does NOT validate cost — call CanEquipSkill first.
     * Returns true if the skill was successfully equipped.
     */
    bool EquipSkill(FName PerkID, const TArray<UPerkData*>& AllPerks, int32 BaseSlots);

    /**
     * Removes the skill from the equipped set.
     * Returns true if it was equipped and has now been removed.
     */
    bool UnequipSkill(FName PerkID);

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
    // Effect queries  (draw only from EquippedSkillPerkIDs)
    // -----------------------------------------------------------------------

    /**
     * Returns all passive FPerkSkillEffect entries from EQUIPPED skill perks only.
     * Unlocked-but-unequipped skills contribute no effects.
     * SkillSlotIncrease effects are intentionally excluded here; use
     * GetSkillSlotCount() to account for those.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    TArray<FPerkSkillEffect> GetAllPassiveEffects(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns all active (non-passive) FPerkSkillEffect entries from EQUIPPED
     * skill perks only.  Applied at race start, removed after their Duration.
     */
    UFUNCTION(BlueprintCallable, Category = "CharacterPerkState")
    TArray<FPerkSkillEffect> GetAllActiveEffects(const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns custom Blueprint effect class pointers from EQUIPPED skill perks only.
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
