// Copyright Epic Games, Inc. All Rights Reserved.

#include "SportsCarNPCRacerData.h"
#include "RacingAISystem/RacingAITypes.h"

USportsCarNPCRacerData::USportsCarNPCRacerData()
{
}

void USportsCarNPCRacerData::PostInitProperties()
{
    Super::PostInitProperties();

    if (HasAnyFlags(RF_ClassDefaultObject)) { return; }

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------
    RacerName   = NSLOCTEXT("NPC", "RivalName",  "Kenji Mori");
    Description = NSLOCTEXT("NPC", "RivalDesc",
        "A local mechanic who knows the mountain road better than anyone. "
        "Calm under pressure but dangerous when pushed � don't let him "
        "get into a rhythm.");

    // -----------------------------------------------------------------------
    // Combat Stats  (moderate baseline � fair first rival)
    // -----------------------------------------------------------------------
    BaseStats.Attack    = 12;
    BaseStats.Defense   = 10;
    BaseStats.Health    = 80;
    BaseStats.Toughness = 10;
    BaseStats.SkillSlots = 3;

    BaseSkillSlots = 3;

    // No perks � stock rival, no special abilities
    PerkIDs.Empty();
    EquippedSkillPerkIDs.Empty();

    // -----------------------------------------------------------------------
    // Vehicle Config  (stock Type-SR � same car as the player starts with)
    // VehicleDefinition must be assigned in the editor to the
    // USportsCarVehicleDefinition data asset.
    // All part levels left at 0 (stock).
    // -----------------------------------------------------------------------
    VehicleConfig.InstalledPartLevels.Empty();
    // GearRatioOverrides intentionally empty � use definition defaults

    // -----------------------------------------------------------------------
    // AI Config
    // -----------------------------------------------------------------------
    // --- Global ---
    AIConfig.ReactionTimeSeconds   = 0.30f;   // slightly slower than a pro
    AIConfig.GlobalAggressionScale = 0.45f;   // moderate � not a pushover

    // --- Rubber-band ---
    AIConfig.RubberBandActivationDistanceCm = 2500.f;
    AIConfig.RubberBandMaxThrottleBonus     = 0.12f;

    // --- Cornering ---
    AIConfig.CorneringConfig.LookaheadDistance    = 2200.f;
    AIConfig.CorneringConfig.CurvatureThreshold   = 0.0009f;
    AIConfig.CorneringConfig.MaxCornerSpeedCmS    = 1800.f;
    AIConfig.CorneringConfig.SplineFollowStrength = 0.80f;

    // --- Behavior Rules ---
    AIConfig.BehaviorRules.Empty();
    // {
    //     // Mirror the player when they try to pass
    //     FAggressionBehaviorRule Mirror;
    //     Mirror.Behavior           = ERacingAIState::BlockingMirror;
    //     Mirror.Condition          = EAIBehaviorCondition::PlayerClose;
    //     Mirror.DistanceThreshold  = 600.f;
    //     Mirror.Probability        = 0.55f;
    //     Mirror.DurationSeconds    = 2.5f;
    //     Mirror.CooldownSeconds    = 5.0f;
    //     Mirror.SteeringIntensity  = 0.55f;
    //     Mirror.ThrottleReduction  = 0.0f;
    //     AIConfig.BehaviorRules.Add(Mirror);
    // }
    // {
    //     // Occasionally nudge when side-by-side
    //     FAggressionBehaviorRule Bump;
    //     Bump.Behavior           = ERacingAIState::Bumping;
    //     Bump.Condition          = EAIBehaviorCondition::PlayerClose;
    //     Bump.DistanceThreshold  = 250.f;
    //     Bump.Probability        = 0.25f;
    //     Bump.DurationSeconds    = 0.8f;
    //     Bump.CooldownSeconds    = 8.0f;
    //     Bump.SteeringIntensity  = 0.45f;
    //     Bump.ThrottleReduction  = 0.0f;
    //     AIConfig.BehaviorRules.Add(Bump);
    // }

    // --- Nitro Rules ---
    AIConfig.NitroRules.Empty();
    // {
    //     // Use nitro when trailing the player
    //     FNitroUsageRule TrailingRule;
    //     TrailingRule.Condition         = EAIBehaviorCondition::Trailing;
    //     TrailingRule.DistanceThreshold = 2000.f;
    //     TrailingRule.NitroThreshold    = 0.5f;
    //     TrailingRule.Probability       = 0.70f;
    //     TrailingRule.CooldownSeconds   = 8.0f;
    //     AIConfig.NitroRules.Add(TrailingRule);
    // }
    // {
    //     // Conserve � don't use nitro when already leading
    //     // (defined as "only fire when NOT leading � so we just skip a Leading rule")
    //     // Use nitro defensively when player is right behind
    //     FNitroUsageRule DefensiveRule;
    //     DefensiveRule.Condition         = EAIBehaviorCondition::PlayerClose;
    //     DefensiveRule.DistanceThreshold = 500.f;
    //     DefensiveRule.NitroThreshold    = 0.6f;
    //     DefensiveRule.Probability       = 0.50f;
    //     DefensiveRule.CooldownSeconds   = 12.0f;
    //     AIConfig.NitroRules.Add(DefensiveRule);
    // }
}
