// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PerkTypes.h"
#include "PerkData.h"
#include "CharacterPerkState.h"
#include "RacingVehicleTypes.h"
#include "PlayerPerkManager.generated.h"

class UVehicleDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerkUnlocked,          UPerkData*, Perk);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillPointsChanged,    int32,      NewTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedSkillsChanged);

/**
 * UPlayerPerkManager
 *
 * Manages the player's skill point bank, unlocked perks, and story flags.
 * Lives on URacingGameInstance for the session lifetime.
 *
 * Responsibilities:
 *   - Validate and execute perk purchases (points, prerequisites, story gates).
 *   - Answer shop-filter queries: which vehicles are visible, which part
 *     levels may be purchased.
 *   - Expose the full perk catalogue to the UI for tree rendering.
 *
 * Designer setup:
 *   Set AllPerks on the GameInstance Blueprint defaults by pointing each
 *   entry to a UPerkData Data Asset.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API UPlayerPerkManager : public UObject
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Perk Catalogue  (designer-populated)
    // -----------------------------------------------------------------------

    /**
     * Complete list of all perks that exist in the game.
     * Populate this from the GameInstance Blueprint defaults.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalogue")
    TArray<TObjectPtr<UPerkData>> AllPerks;

    // -----------------------------------------------------------------------
    // Player State  (saved)
    // -----------------------------------------------------------------------

    /** Unspent skill points available for purchasing perks. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 SkillPointBank = 0;

    /**
     * The baseline number of skill slots before any SkillSlotIncrease perk bonuses.
     * Set this from the game mode or difficulty level before the session starts.
     * Default is 3 — a reasonable starting point for most configurations.
     */
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Skills")
    int32 BaseSkillSlots = 3;

    /** The player's unlocked perk set and derived stat state. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Perks")
    TObjectPtr<UCharacterPerkState> PerkState;

    /**
     * Named flags set by the story/progression system.
     * Perks with matching StoryRequirements become unlockable once the flag is present.
     * Convention: "story.act1.complete", "rival.kenji.defeated", etc.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Story")
    TSet<FName> AchievedStoryFlags;

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnPerkUnlocked OnPerkUnlocked;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillPointsChanged OnSkillPointsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEquippedSkillsChanged OnEquippedSkillsChanged;

    // -----------------------------------------------------------------------
    // Initialisation  (called by URacingGameInstance::Init)
    // -----------------------------------------------------------------------

    void Initialise();

    // -----------------------------------------------------------------------
    // Skill Points
    // -----------------------------------------------------------------------

    /** Awards skill points (e.g. from completing races or story events). */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    void AddSkillPoints(int32 Amount);

    // -----------------------------------------------------------------------
    // Perk Purchase
    // -----------------------------------------------------------------------

    /**
     * Returns true if the perk can be purchased right now:
     *   - Not already unlocked.
     *   - All prerequisite perks are unlocked.
     *   - All story flags are present.
     *   - Player has enough skill points.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    bool CanUnlockPerk(UPerkData* Perk) const;

    /**
     * Purchases the perk: deducts points, adds to PerkState, broadcasts event.
     * Returns true on success.
     */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    bool UnlockPerk(UPerkData* Perk);

    // -----------------------------------------------------------------------
    // Story Flags
    // -----------------------------------------------------------------------

    /** Records that a story milestone has been reached. */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    void AddStoryFlag(FName Flag);

    /** Returns true if the given story flag has been achieved. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    bool HasStoryFlag(FName Flag) const;

    // -----------------------------------------------------------------------
    // Skill Equipping
    // -----------------------------------------------------------------------

    /** Returns the total skill slot count (BaseSkillSlots + any SkillSlotIncrease bonuses). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    int32 GetSkillSlotCount() const;

    /** Returns true if the given skill perk is currently equipped. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    bool IsSkillEquipped(FName PerkID) const;

    /**
     * Returns true if the skill can be equipped:
     *   - Perk is unlocked, is a Driver skill perk, is not already equipped,
     *     and a free slot is available.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    bool CanEquipSkill(UPerkData* Perk) const;

    /**
     * Equips the skill into the next available slot.
     * Broadcasts OnEquippedSkillsChanged on success.
     * Returns true if the skill was equipped.
     */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    bool EquipSkill(UPerkData* Perk);

    /**
     * Removes the skill from the equipped set.
     * Broadcasts OnEquippedSkillsChanged on success.
     * Returns true if the skill was previously equipped.
     */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    bool UnequipSkill(UPerkData* Perk);

    // -----------------------------------------------------------------------
    // Shop Filter Queries
    // -----------------------------------------------------------------------

    /**
     * Returns all UVehicleDefinition assets for vehicles whose unlock perk
     * is in the player's unlocked set.  This is the list the shop renders.
     */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    TArray<UVehicleDefinition*> GetUnlockedVehicles() const;

    /**
     * Returns true if the player has unlocked the tuning perk that grants
     * purchasing rights for the given part slot at the given level index.
     * Used by UVehicleInventory::CanInstallPart to gate the shop.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    bool IsPartLevelPurchasable(EPartSlot Slot, int32 Level) const;

    // -----------------------------------------------------------------------
    // Convenience wrappers forwarding to PerkState
    // -----------------------------------------------------------------------

    /** Returns the player's current character level (count of stat perks unlocked). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PlayerPerkManager")
    int32 GetPlayerLevel() const;

    /** Returns the player's resolved combat stats. */
    UFUNCTION(BlueprintCallable, Category = "PlayerPerkManager")
    FDriverStatBlock GetPlayerStats(const FDriverStatBlock& BaseStats) const;

private:

    /** Converts AllPerks (TObjectPtr array) to a raw pointer array for CharacterPerkState helpers. */
    TArray<UPerkData*> GetRawPerkArray() const;
};
