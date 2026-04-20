// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RacingAITypes.generated.h"

// ---------------------------------------------------------------------------
// AI States
// ---------------------------------------------------------------------------

/**
 * The set of states the NPC racing AI state machine can occupy.
 * Multiple states can be active simultaneously where noted � the controller
 * layers them (e.g. Cornering modifies throttle on top of any other state).
 */
UENUM(BlueprintType)
enum class ERacingAIState : uint8
{
    /** Default: follow the racing line at full pace. */
    Racing              UMETA(DisplayName = "Racing"),

    /**
     * Patrol mode: follow the assigned patrol spline at low speed indefinitely.
     * Active before a race starts.  Replaced by Racing when StartRace() is called.
     */
    Idle                UMETA(DisplayName = "Idle Patrol"),

    /**
     * Mirrors the player's lateral offset on the road to deny the overtake line.
     * The NPC tracks the player's side-to-side position and copies it.
     */
    BlockingMirror      UMETA(DisplayName = "Blocking: Mirror"),

    /**
     * Slows slightly and drifts toward the player's current lane
     * to narrow the gap without an aggressive mirror.
     */
    BlockingSlowDrift   UMETA(DisplayName = "Blocking: Slow Drift"),

    /**
     * Steers into the player to make contact.
     * Intensity scaled by the aggression rule's magnitude.
     */
    Bumping             UMETA(DisplayName = "Bumping"),

    /**
     * Activates the nitro system.
     * Exits automatically when nitro is depleted or the trigger condition clears.
     */
    UsingNitro          UMETA(DisplayName = "Using Nitro"),

    /**
     * Reduces throttle and steers toward the racing line apex.
     * Applied as a modifier layer on top of the base state � does not
     * replace it, but overrides throttle and steering while active.
     */
    Cornering           UMETA(DisplayName = "Cornering"),
};

// ---------------------------------------------------------------------------
// Behavior Trigger Conditions
// ---------------------------------------------------------------------------

/**
 * The race situation that must be true for a behavior rule to be considered.
 * Multiple rules can share the same condition; all eligible rules are evaluated
 * each tick and any that pass their probability roll activate simultaneously.
 */
UENUM(BlueprintType)
enum class EAIBehaviorCondition : uint8
{
    /** Always eligible � rule is evaluated every tick. */
    Always              UMETA(DisplayName = "Always"),

    /** NPC is trailing the player by at least DistanceThreshold. */
    Trailing            UMETA(DisplayName = "Trailing"),

    /** NPC is ahead of the player by at least DistanceThreshold. */
    Leading             UMETA(DisplayName = "Leading"),

    /** Player is within DistanceThreshold behind the NPC (close pursuit). */
    PlayerClose         UMETA(DisplayName = "Player Close"),

    /** Player is within DistanceThreshold ahead of the NPC (close approach). */
    PlayerCloseAhead    UMETA(DisplayName = "Player Close Ahead"),

    /** NPC's nitro is above NitroThreshold (0�1 fraction). */
    NitroAvailable      UMETA(DisplayName = "Nitro Available"),

    /** NPC's nitro is below NitroThreshold (0�1 fraction). */
    NitroLow            UMETA(DisplayName = "Nitro Low"),

    /** NPC's current speed is below SpeedThreshold (km/h). */
    SpeedLow            UMETA(DisplayName = "Speed Low"),
};

// ---------------------------------------------------------------------------
// Nitro Usage Rule
// ---------------------------------------------------------------------------

/**
 * Defines one condition under which the NPC will consider activating nitro.
 * All rules are evaluated each tick; any that pass their probability roll
 * will trigger a nitro activation.
 */
USTRUCT(BlueprintType)
struct FNitroUsageRule
{
    GENERATED_BODY()

    /** The race situation that must be true before this rule is checked. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NitroRule")
    EAIBehaviorCondition Condition = EAIBehaviorCondition::Trailing;

    /**
     * Distance in cm used by distance-based conditions (Trailing, Leading,
     * PlayerClose, PlayerCloseAhead).  Ignored for non-distance conditions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NitroRule",
        meta = (ClampMin = "0.0"))
    float DistanceThreshold = 1000.0f;

    /**
     * Nitro fraction threshold (0�1) used by NitroAvailable / NitroLow conditions.
     * Ignored for non-nitro conditions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NitroRule",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float NitroThreshold = 0.5f;

    /**
     * Probability (0�1) that the NPC activates nitro when this condition is met.
     * Evaluated once per reevaluation interval, not every frame.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NitroRule",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Probability = 0.8f;

    /**
     * Minimum seconds between successive activations triggered by this rule.
     * Prevents rapid on-off nitro toggling.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NitroRule",
        meta = (ClampMin = "0.0"))
    float CooldownSeconds = 5.0f;
};

// ---------------------------------------------------------------------------
// Aggression Behavior Rule
// ---------------------------------------------------------------------------

/**
 * Defines one aggressive behavior (bumping, blocking) that the NPC may perform.
 * Multiple rules can be active simultaneously.
 */
USTRUCT(BlueprintType)
struct FAggressionBehaviorRule
{
    GENERATED_BODY()

    /** The behavior this rule activates when its conditions are met. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule")
    ERacingAIState Behavior = ERacingAIState::BlockingMirror;

    /** The race situation that must be true before this rule is evaluated. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule")
    EAIBehaviorCondition Condition = EAIBehaviorCondition::PlayerClose;

    /**
     * Distance in cm relevant to the condition.
     * For PlayerClose / PlayerCloseAhead: player must be within this range.
     * For Trailing / Leading: gap must be at least this large.
     * Ignored for non-distance conditions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0"))
    float DistanceThreshold = 500.0f;

    /**
     * Speed threshold in cm/s used by SpeedLow condition.
     * Ignored for non-speed conditions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0"))
    float SpeedThreshold = 1000.0f;

    /**
     * Probability (0�1) of activating this behavior when the condition is met.
     * Rolled once per reevaluation interval.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Probability = 0.5f;

    /**
     * How long in seconds this behavior stays active once triggered,
     * regardless of whether the condition remains true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0"))
    float DurationSeconds = 2.0f;

    /**
     * Minimum seconds before this rule can trigger again after its duration ends.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0"))
    float CooldownSeconds = 4.0f;

    /**
     * Steering intensity applied when executing this behavior (0�1).
     * 1.0 = full steering input toward the target; lower values = gentler move.
     * Relevant for Bumping and BlockingSlowDrift.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SteeringIntensity = 0.6f;

    /**
     * Throttle reduction applied during this behavior (0�1, where 0 = no reduction).
     * Relevant for BlockingSlowDrift.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BehaviorRule",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ThrottleReduction = 0.0f;
};

// ---------------------------------------------------------------------------
// Cornering Config
// ---------------------------------------------------------------------------

/**
 * Controls how the NPC handles upcoming corners detected via the racing spline.
 */
USTRUCT(BlueprintType)
struct FCorneringConfig
{
    GENERATED_BODY()

    /**
     * Distance in cm ahead to scan for corners.
     * Must be large enough for the car to brake. At 100 km/h braking at ~1g,
     * stopping distance is ~40 m � keep this above 5000 cm.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "100.0"))
    float LookaheadDistance = 5000.0f;

    /**
     * Curvature above which cornering logic activates (radians/cm = 1/radius in cm).
     * 0.0003 = ~33 m radius curve.  0.0008 = ~12.5 m radius.
     * Lower this to react to gentler bends.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "0.0"))
    float CurvatureThreshold = 0.0003f;

    /**
     * Maximum lateral acceleration the car can sustain (cm/s�).
     * Physics-based max corner speed = sqrt(AILateralAccelCmS2 / curvature).
     * 900 cm/s� (~0.9g) is a good starting point for the sports car.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "100.0"))
    float AILateralAccelCmS2 = 900.0f;

    /**
     * Optional hard cap on corner speed (cm/s).
     * Set to 0 to rely purely on the physics-based calculation above.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "0.0"))
    float MaxCornerSpeedCmS = 0.0f;

    /**
     * How strongly the NPC steers toward the spline tangent while cornering (0-1).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SplineFollowStrength = 0.85f;

    /**
     * Distance (cm) ahead on the spline used to sample the spline tangent for
     * steering.  The car aligns its heading toward where the track is *going*
     * at this lookahead distance, giving predictive turn-in for corners.
     * 800-1500 cm works well at typical race speeds (~80-120 km/h).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "50.0"))
    float SteeringLookaheadCm = 900.0f;

    /**
     * Lateral offset (cm) at which the lateral-correction signal reaches its
     * maximum contribution.  At offsets smaller than this the correction is
     * proportionally smaller, so the car eases back to center rather than
     * snapping hard toward the line.  ~500-800 cm (5-8 m) is a good range;
     * increase to make the car more tolerant of being off-center.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "50.0"))
    float LateralCorrectionScaleCm = 600.0f;

    /**
     * How much the lateral-correction signal is blended into the final steering
     * output (0-1).  Keep this low (0.2-0.35) to avoid oscillation: the
     * heading component already steers toward the line; this just provides a
     * gentle nudge when the car is significantly off center.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cornering",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LateralCorrectionWeight = 0.25f;
};

// ---------------------------------------------------------------------------
// Per-Difficulty AI Override
// ---------------------------------------------------------------------------

/**
 * Multipliers applied to AI config values based on a global difficulty level.
 * The game mode sets the active difficulty; the AI controller applies these
 * on top of the NPC's base config.
 */
USTRUCT(BlueprintType)
struct FAIDifficultyOverride
{
    GENERATED_BODY()

    /**
     * Scales the NPC's reaction time.
     * < 1.0 = faster reactions; > 1.0 = slower.
     * Applied multiplicatively to FRacingAIConfig::ReactionTimeSeconds.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "0.1"))
    float ReactionTimeMultiplier = 1.0f;

    /**
     * Scales the probability of all behavior and nitro rules.
     * < 1.0 = less aggressive; > 1.0 = more aggressive (capped at 1.0 per rule).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "0.0"))
    float AggressionMultiplier = 1.0f;

    /**
     * Additional top speed bonus applied as a fraction of max speed (0�1).
     * Stacks with rubber-band; use to make higher difficulties genuinely faster.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpeedBonusFraction = 0.0f;
};

// ---------------------------------------------------------------------------
// Lane Driving Config
// ---------------------------------------------------------------------------

/**
 * Lane-following and obstacle-avoidance tuning embedded in FRacingAIConfig.
 *
 * The NPC drives its CurrentLaneSpline (picked from the CourseSplineActor it
 * was assigned) rather than the raw spline centerline.  When an obstacle is
 * detected ahead, EvaluateLaneChange() selects the clearest sibling lane and
 * begins a smooth blend transition.
 */
USTRUCT(BlueprintType)
struct FLaneConfig
{
    GENERATED_BODY()

    /**
     * Time in seconds to blend steering from the old lane to the new one.
     * During this window ComputeSplineSteeringInput lerps between the two
     * lane results so the path change is smooth rather than a snap.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane",
        meta = (ClampMin = "0.1"))
    float LaneChangeDurationSec = 1.5f;

    /**
     * How far ahead (cm) to sweep for blocking vehicles when evaluating
     * whether a lane change is needed.  Should be at least one car-length
     * of reaction distance at patrol speed (~1500 cm covers ~1 second at
     * 35 MPH).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane",
        meta = (ClampMin = "100.0"))
    float ObstacleLookaheadCm = 1500.0f;

    /**
     * Radius (cm) of the capsule sweep used to detect blocking vehicles.
     * Should be roughly half a lane width so the sweep catches cars that
     * share the lane without triggering on cars in adjacent lanes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lane",
        meta = (ClampMin = "10.0"))
    float ObstacleSweepRadiusCm = 180.0f;
};

// ---------------------------------------------------------------------------
// Racing AI Config  (embedded in UNPCRacerData)
// ---------------------------------------------------------------------------

/**
 * The complete designer-facing AI configuration for one NPC racer.
 * Embedded in UNPCRacerData so it lives in the same Data Asset.
 *
 * Global settings affect all behavior.  Per-NPC difficulty overrides stack on
 * top of the global difficulty setting applied by the game mode � so each NPC
 * can be individually more or less reactive regardless of difficulty.
 */
USTRUCT(BlueprintType)
struct FRacingAIConfig
{
    GENERATED_BODY()

    // -----------------------------------------------------------------------
    // Idle Patrol
    // -----------------------------------------------------------------------

    /**
     * Target speed for idle patrol in MPH.
     * The AI uses a proportional throttle controller to hold this speed,
     * so it naturally adds throttle uphill and lifts off downhill.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Idle",
        meta = (ClampMin = "0.0"))
    float IdleTargetSpeedMPH = 35.0f;

    /**
     * How strongly the NPC steers toward the patrol spline during idle (0-1).
     * Lower values give smoother, lazier corrections.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Idle",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IdleSplineFollowStrength = 0.65f;

    // -----------------------------------------------------------------------
    // Global Behaviour
    // -----------------------------------------------------------------------

    /**
     * Seconds between behavior reevaluation ticks.
     * Lower = more responsive; higher = slower reactions and more variation.
     * Difficulty overrides are applied multiplicatively on top of this.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Global",
        meta = (ClampMin = "0.05"))
    float ReactionTimeSeconds = 0.25f;

    /**
     * Global aggression scale applied to all behavior rule probabilities (0�1).
     * Set to 0 for a purely passive NPC; 1 for maximum use of all rules.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Global",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GlobalAggressionScale = 0.5f;

    // -----------------------------------------------------------------------
    // Rubber-Band
    // -----------------------------------------------------------------------

    /**
     * Distance in cm the player must be ahead before rubber-band kicks in.
     * Set to 0 to disable rubber-banding for this NPC.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|RubberBand",
        meta = (ClampMin = "0.0"))
    float RubberBandActivationDistanceCm = 3000.0f;

    /**
     * Maximum additional throttle fraction (0�1) granted by rubber-band.
     * Applied linearly from 0 at activation distance to full bonus at 2x that distance.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|RubberBand",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RubberBandMaxThrottleBonus = 0.15f;

    // -----------------------------------------------------------------------
    // Behavior Rules
    // -----------------------------------------------------------------------

    /**
     * Ordered list of aggression behavior rules.
     * All rules whose conditions are met are evaluated each reaction tick.
     * Multiple can activate simultaneously (e.g. bumping while leading).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Behaviors")
    TArray<FAggressionBehaviorRule> BehaviorRules;

    // -----------------------------------------------------------------------
    // Nitro Rules
    // -----------------------------------------------------------------------

    /**
     * Conditions under which this NPC will consider activating nitro.
     * Multiple rules can trigger simultaneously.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Nitro")
    TArray<FNitroUsageRule> NitroRules;

    // -----------------------------------------------------------------------
    // Cornering
    // -----------------------------------------------------------------------

    /** How this NPC reads and responds to upcoming corners in the spline. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Cornering")
    FCorneringConfig CorneringConfig;

    /** Lane-following and obstacle-avoidance settings. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Lane")
    FLaneConfig LaneConfig;

    // -----------------------------------------------------------------------
    // Per-NPC Difficulty Override
    // -----------------------------------------------------------------------

    /**
     * NPC-specific difficulty scaling applied on top of the global difficulty.
     * Use this to make a specific rival inherently faster or more aggressive
     * regardless of difficulty setting.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Difficulty")
    FAIDifficultyOverride NPCDifficultyOverride;
};
