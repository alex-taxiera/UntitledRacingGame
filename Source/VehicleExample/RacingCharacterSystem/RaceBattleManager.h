// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PerkTypes.h"
#include "PerkData.h"
#include "CharacterPerkState.h"
#include "PerkSkillEffect.h"
#include "RaceBattleManager.generated.h"

class UPlayerPerkManager;
class UNPCRacerData;

// ---------------------------------------------------------------------------
// Delegates
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged,
    bool, bIsPlayer, float, NewHealth, float, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRacerDefeated,
    bool, bPlayerDefeated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimedEffectExpired,
    bool, bIsPlayer, FPerkSkillEffect, Effect);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleEnded);

// ---------------------------------------------------------------------------
// URaceBattleManager
// ---------------------------------------------------------------------------

/**
 * URaceBattleManager
 *
 * Manages the full runtime state of one race battle between the player
 * and a single NPC opponent.
 *
 * Responsibilities:
 *   - Initialise both racers (resolve stats, apply effects, set max health).
 *   - Advance elapsed time and tick timed effects each frame.
 *   - Handle health changes from distance-based damage, wall collisions, and healing.
 *   - Provide display queries for the HUD (names, health, active skills).
 *
 * Lifetime:
 *   Create one instance per battle via NewObject and call StartBattle().
 *   The owning game mode or battle controller is responsible for calling
 *   Tick() each frame and EndBattle() when the race concludes.
 *
 * This object does NOT drive the race itself (speed, distance, AI steering).
 *   That lives in the pawn and movement systems.  This object only manages
 *   the health/stats/effects layer.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API URaceBattleManager : public UObject
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Events
    // -----------------------------------------------------------------------

    /** Fired whenever either racer's health changes. */
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnHealthChanged OnHealthChanged;

    /** Fired when a racer's health reaches 0. */
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnRacerDefeated OnRacerDefeated;

    /** Fired when a timed skill effect expires for either racer. */
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnTimedEffectExpired OnTimedEffectExpired;

    /** Fired once when StartBattle completes successfully. */
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleStarted OnBattleStarted;

    /** Fired once when EndBattle is called. */
    UPROPERTY(BlueprintAssignable, Category = "Battle|Events")
    FOnBattleEnded OnBattleEnded;

    // -----------------------------------------------------------------------
    // Battle State  (read-only from Blueprint)
    // -----------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    FRacerBattleState PlayerBattleState;

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    FRacerBattleState OpponentBattleState;

    /** Seconds elapsed since StartBattle was called. Only advances while bBattleActive. */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    float RaceElapsedSeconds = 0.0f;

    /** True between StartBattle and EndBattle. */
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    bool bBattleActive = false;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * Initialises both racers and begins the battle.
     *
     * @param InPlayerManager   The player's perk manager (for stats, equipped skills).
     * @param InPlayerBaseStats The player's base stats before perk bonuses.
     * @param InPlayerName      Display name for the player character.
     * @param InOpponent        The NPC racer data asset.
     * @param InAllPerks        Full perk catalogue used to resolve effects.
     * @param EffectOuter       Object to use as Outer when instantiating BP effects.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void StartBattle(UPlayerPerkManager* InPlayerManager,
                     const FDriverStatBlock& InPlayerBaseStats,
                     const FText& InPlayerName,
                     UNPCRacerData* InOpponent,
                     const TArray<UPerkData*>& InAllPerks,
                     UObject* EffectOuter);

    /**
     * Ends the battle, removes all live effects, and broadcasts OnBattleEnded.
     * Safe to call even if the battle is not active.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void EndBattle();

    /**
     * Advances the race timer and ticks all active timed effects.
     * Call this every frame from your game mode or battle controller.
     * Does nothing if bBattleActive is false.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void Tick(float DeltaSeconds);

    // -----------------------------------------------------------------------
    // Health / Damage
    // -----------------------------------------------------------------------

    /**
     * Applies distance-based damage to the racer that is behind.
     *
     * Damage formula:
     *   BaseDamage = Attacker.Attack * DistanceFraction
     *   Reduction  = Defender.Defense + (Defender.DamageReduction effects)
     *   FinalDamage = max(0, BaseDamage - Reduction)
     *
     * @param bPlayerIsBehind  True if the player is the one taking damage.
     * @param DistanceFraction Normalised distance gap (0 = same position, 1 = max gap).
     *                         Callers should derive this from actual race distance data.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ApplyDistanceDamage(bool bPlayerIsBehind, float DistanceFraction);

    /**
     * Applies collision damage to the racer that hit a wall or barrier.
     *
     * Damage formula:
     *   BaseDamage = ImpactForce (caller-provided)
     *   Reduction  = Racer.Toughness + (Racer.WallDamageReduction effects)
     *   FinalDamage = max(0, BaseDamage - Reduction)
     *
     * @param bPlayerHit    True if the player hit the wall.
     * @param ImpactForce   Raw collision force magnitude (game-unit scale).
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void ApplyWallDamage(bool bPlayerHit, float ImpactForce);

    /**
     * Heals the given racer by Amount, capped at their MaxHealth.
     * Broadcasts OnHealthChanged.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void HealRacer(bool bPlayer, float Amount);

    /**
     * Called when the given racer activates their nitro system.
     * Applies HealthOnNitro heal from all active passive effects.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle")
    void OnNitroUsed(bool bPlayer);

    // -----------------------------------------------------------------------
    // Display Queries
    // -----------------------------------------------------------------------

    /** Returns the display name of the given racer. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle|Display")
    FText GetDisplayName(bool bPlayer) const;

    /** Returns the current health of the given racer. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle|Display")
    float GetCurrentHealth(bool bPlayer) const;

    /** Returns the maximum health of the given racer. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle|Display")
    float GetMaxHealth(bool bPlayer) const;

    /** Returns current health as a 0.0–1.0 fraction of max health. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle|Display")
    float GetHealthPercent(bool bPlayer) const;

    /**
     * Returns the display names of all equipped skill perks for the given racer.
     * Looks up each perk ID in AllPerks and returns its DisplayName.
     * Order matches EquippedSkillPerkIDs.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle|Display")
    TArray<FText> GetEquippedSkillNames(bool bPlayer,
                                         const TArray<UPerkData*>& AllPerks) const;

    /**
     * Returns the display names of timed effects that are currently active
     * (i.e. not yet expired) for the given racer.
     */
    UFUNCTION(BlueprintCallable, Category = "Battle|Display")
    TArray<FText> GetActiveTimedEffectNames(bool bPlayer,
                                              const TArray<UPerkData*>& AllPerks) const;

    /** Returns the elapsed race time in whole seconds. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle|Display")
    float GetRaceElapsedSeconds() const { return RaceElapsedSeconds; }

private:

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /** Builds one FRacerBattleState from a perk state, resolved stats, name, and effect outer. */
    void InitialiseRacerState(FRacerBattleState& OutState,
                               const FText& Name,
                               const FDriverStatBlock& ResolvedStats,
                               UCharacterPerkState* PerkState,
                               const TArray<UPerkData*>& AllPerks,
                               UObject* EffectOuter);

    /** Applies a health delta (negative = damage) to the chosen racer, clamps, broadcasts. */
    void ModifyHealth(FRacerBattleState& State, bool bIsPlayer, float Delta);

    /** Advances timed effects for one racer, expiring any whose duration is up. */
    void TickTimedEffects(FRacerBattleState& State, bool bIsPlayer, float DeltaSeconds);

    /** Returns a mutable reference to the correct racer state. */
    FRacerBattleState& GetState(bool bPlayer)
    {
        return bPlayer ? PlayerBattleState : OpponentBattleState;
    }

    const FRacerBattleState& GetState(bool bPlayer) const
    {
        return bPlayer ? PlayerBattleState : OpponentBattleState;
    }
};
