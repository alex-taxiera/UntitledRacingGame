// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingAIController.h"
#include "RacingSplineComponent.h"
#include "CourseSplineActor.h"
#include "NPCRacerData.h"
#include "VehicleExamplePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Math/UnrealMathUtility.h"
#include "EngineUtils.h"

ARacingAIController::ARacingAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f; // ticked every frame; reaction gated internally
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void ARacingAIController::SetRacerData(UNPCRacerData* InRacerData)
{
    RacerData = InRacerData;

    if (!RacerData) { return; }

    const FRacingAIConfig& Cfg = RacerData->AIConfig;

    EffectiveReactionTime    = Cfg.ReactionTimeSeconds;
    EffectiveAggressionScale = Cfg.GlobalAggressionScale;
    EffectiveRubberBandBonus = Cfg.RubberBandMaxThrottleBonus;

    // Initialise per-rule cooldown arrays
    BehaviorRuleCooldowns.Init(0.0f, Cfg.BehaviorRules.Num());
    NitroRuleCooldowns.Init(0.0f, Cfg.NitroRules.Num());
}

void ARacingAIController::SetRacingSpline(URacingSplineComponent* InSpline)
{
    RacingSpline = InSpline;
}

void ARacingAIController::SetPlayerPawn(AVehicleExamplePawn* InPlayerPawn)
{
    PlayerPawn = InPlayerPawn;
}

void ARacingAIController::ApplyDifficultyOverride(const FAIDifficultyOverride& GlobalOverride)
{
    if (!RacerData) { return; }

    const FAIDifficultyOverride& NPCOverride = RacerData->AIConfig.NPCDifficultyOverride;
    const FRacingAIConfig& Cfg = RacerData->AIConfig;

    // Combine global and NPC-specific overrides multiplicatively
    const float ReactionMult    = GlobalOverride.ReactionTimeMultiplier
                                * NPCOverride.ReactionTimeMultiplier;
    const float AggressionMult  = GlobalOverride.AggressionMultiplier
                                * NPCOverride.AggressionMultiplier;
    const float SpeedBonus      = FMath::Clamp(
                                    GlobalOverride.SpeedBonusFraction
                                    + NPCOverride.SpeedBonusFraction, 0.0f, 1.0f);

    EffectiveReactionTime    = FMath::Max(0.05f, Cfg.ReactionTimeSeconds * ReactionMult);
    EffectiveAggressionScale = FMath::Clamp(Cfg.GlobalAggressionScale * AggressionMult, 0.0f, 1.0f);
    // Fold speed bonus into rubber-band bonus (same throttle path)
    EffectiveRubberBandBonus = FMath::Clamp(
        Cfg.RubberBandMaxThrottleBonus + SpeedBonus, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Race lifecycle
// ---------------------------------------------------------------------------

void ARacingAIController::StartRace()
{
    bIdleActive = false;
    bRaceActive = true;
    TimeSinceLastReaction = 0.0f;
    TimeInCurrentState    = 0.0f;
    ActiveStates.Reset();
    ActiveStates.Add(ERacingAIState::Racing);

    // Initialise lane state to the patrol spline (or racing spline as fallback).
    // The NPC drives this lane until EvaluateLaneChange() picks a sibling.
    CurrentLaneSpline  = PatrolSpline ? PatrolSpline : RacingSpline;
    PreviousLaneSpline = nullptr;
    LaneBlendAlpha     = 1.0f;

    // Wake the physics body immediately so throttle takes effect on frame 1.
    if (OwnPawn && OwnPawn->GetMesh())
    {
        OwnPawn->GetMesh()->WakeAllRigidBodies();
    }
}

void ARacingAIController::EndRace()
{
    bRaceActive = false;
    ActiveStates.Empty();
    StateDurationRemaining.Empty();
    SetThrottle(0.0f);
    SetSteering(0.0f);
    SetBrake(0.0f);
}

void ARacingAIController::StartIdle()
{
    if (bRaceActive) { return; }
    bIdleActive = true;
    ActiveStates.Reset();
    ActiveStates.Add(ERacingAIState::Idle);

    // Initialise lane state for patrol mode.
    CurrentLaneSpline  = PatrolSpline ? PatrolSpline : RacingSpline;
    PreviousLaneSpline = nullptr;
    LaneBlendAlpha     = 1.0f;
}

void ARacingAIController::StopIdle()
{
    bIdleActive = false;
    ActiveStates.Remove(ERacingAIState::Idle);
    SetThrottle(0.0f);
    SetSteering(0.0f);
    SetBrake(0.0f);
}

void ARacingAIController::SetPatrolSpline(URacingSplineComponent* InSpline)
{
    PatrolSpline = InSpline;
}

// ---------------------------------------------------------------------------
// AActor / AController
// ---------------------------------------------------------------------------

void ARacingAIController::BeginPlay()
{
    Super::BeginPlay();

    // Auto-find the racing spline if none was explicitly assigned
    if (!RacingSpline)
    {
        RacingSpline = URacingSplineComponent::FindInWorld(GetWorld());
    }
}

void ARacingAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    OwnPawn = Cast<AVehicleExamplePawn>(InPawn);
}

void ARacingAIController::OnUnPossess()
{
    OwnPawn = nullptr;
    Super::OnUnPossess();
}

// ---------------------------------------------------------------------------
// Tick � gates on reaction time
// ---------------------------------------------------------------------------

// We override Tick via the standard AActor path.
// ARacingAIController inherits Tick from AActor through AController.
void ARacingAIController::Tick(float DeltaSeconds)  // NOLINT
{
    Super::Tick(DeltaSeconds);

    if (!bRaceActive && !bIdleActive) { return; }

    // Decay cooldowns every frame
    for (float& CD : BehaviorRuleCooldowns) { CD = FMath::Max(0.0f, CD - DeltaSeconds); }
    for (float& CD : NitroRuleCooldowns)    { CD = FMath::Max(0.0f, CD - DeltaSeconds); }

    // Decay per-state duration timers
    for (auto It = StateDurationRemaining.CreateIterator(); It; ++It)
    {
        It->Value -= DeltaSeconds;
        if (It->Value <= 0.0f)
        {
            ActiveStates.Remove(It->Key);
            It.RemoveCurrent();
        }
    }

    TimeSinceLastReaction += DeltaSeconds;
    TimeInCurrentState    += DeltaSeconds;

    if (TimeSinceLastReaction >= EffectiveReactionTime)
    {
        TimeSinceLastReaction = 0.0f;
        ReactionTick(DeltaSeconds);
    }

    // Advance the lane-change blend every frame so the transition is smooth
    // regardless of the reaction tick interval.
    TickLaneBlend(DeltaSeconds);

    // Execute chosen states every frame so inputs stay smooth
    ExecuteState();
}

// ---------------------------------------------------------------------------
// Core state machine
// ---------------------------------------------------------------------------

void ARacingAIController::ReactionTick(float DeltaSeconds)
{
    UpdateContext();
    EvaluateState();
}

void ARacingAIController::UpdateContext()
{
    if (!OwnPawn) { return; }

    Context.OwnPawn      = OwnPawn;
    Context.PlayerPawn   = PlayerPawn;
    Context.RacingSpline = RacingSpline;

    // Own speed and gear
    if (UChaosWheeledVehicleMovementComponent* Move =
        OwnPawn->FindComponentByClass<UChaosWheeledVehicleMovementComponent>())
    {
        Context.OwnSpeedCmS = Move->GetForwardSpeed(); // cm/s
        Context.OwnGear     = Move->GetCurrentGear();
    }

    // Nitro fraction � read from OwnedVehicle via pawn if available
    // OwnedVehicle.CurrentNitro / EffectiveStats.NitroCapacity
    // For now we use a safe default; wire this up once the pawn exposes NitroFraction
    Context.OwnNitroFraction = 0.5f;

    // Spline-based position and corner sensing
    if (RacingSpline)
    {
        const FVector OwnLocation = OwnPawn->GetActorLocation();
        Context.OwnSplineDistance = RacingSpline->GetNearestSplineDistance(OwnLocation);
        Context.SplineTangent     = RacingSpline->GetDirectionAtDistance(Context.OwnSplineDistance);

        // Lateral offset: project (own pos - spline pos) onto right vector
        const FVector SplinePos   = RacingSpline->GetLocationAtDistance(Context.OwnSplineDistance);
        const FVector Right       = FVector::CrossProduct(Context.SplineTangent, FVector::UpVector);
        Context.OwnLateralOffsetCm = FVector::DotProduct(OwnLocation - SplinePos, Right);

        // Lookahead curvature
        const float Lookahead = RacerData
                              ? RacerData->AIConfig.CorneringConfig.LookaheadDistance
                              : 5000.0f;
        // Sample the WORST curvature across the full window so the car
        // brakes for the tightest point ahead, not just one arbitrary sample.
        Context.UpcomingCurvature = RacingSpline->GetMaxCurvatureInRange(
            Context.OwnSplineDistance, Lookahead);

        const float CurvThreshold = RacerData
                                  ? RacerData->AIConfig.CorneringConfig.CurvatureThreshold
                                  : 0.0008f;
        Context.bCornerAhead = (Context.UpcomingCurvature >= CurvThreshold);

        // Player relative distance
        if (PlayerPawn)
        {
            const FVector PlayerLocation  = PlayerPawn->GetActorLocation();
            const float   PlayerSplineDist = RacingSpline->GetNearestSplineDistance(PlayerLocation);
            Context.DistanceToPlayerCm    = PlayerSplineDist - Context.OwnSplineDistance;
            Context.bPlayerAhead          = Context.DistanceToPlayerCm > 0.0f;

            const FVector PlayerSplinePos = RacingSpline->GetLocationAtDistance(PlayerSplineDist);
            Context.PlayerLateralOffsetCm = FVector::DotProduct(
                PlayerLocation - PlayerSplinePos, Right);
        }
    }
    else if (PlayerPawn)
    {
        // Fallback: straight-line distance, no lateral data
        const FVector Delta           = PlayerPawn->GetActorLocation() - OwnPawn->GetActorLocation();
        const float   ForwardDot      = FVector::DotProduct(Delta, OwnPawn->GetActorForwardVector());
        Context.DistanceToPlayerCm    = ForwardDot;
        Context.bPlayerAhead          = ForwardDot > 0.0f;
        Context.PlayerLateralOffsetCm = 0.0f;
        Context.SplineTangent         = OwnPawn->GetActorForwardVector();
    }

    // --- Current lane spline position ---
    // Populate CurrentLaneSplineDistance and CurrentLaneLateralOffsetCm from
    // the lane the NPC is actually driving, which may differ from RacingSpline.
    if (CurrentLaneSpline && CurrentLaneSpline != RacingSpline)
    {
        const FVector OwnLocation        = OwnPawn->GetActorLocation();
        Context.CurrentLaneSplineDistance = CurrentLaneSpline->GetNearestSplineDistance(OwnLocation);
        const FVector LanePos             = CurrentLaneSpline->GetLocationAtDistance(
                                                Context.CurrentLaneSplineDistance);
        const FVector LaneTangent         = CurrentLaneSpline->GetDirectionAtDistance(
                                                Context.CurrentLaneSplineDistance);
        const FVector LaneRight           = FVector::CrossProduct(LaneTangent, FVector::UpVector);
        Context.CurrentLaneLateralOffsetCm = FVector::DotProduct(OwnLocation - LanePos, LaneRight);
    }
    else
    {
        // Lane is the same as the reference spline — reuse already-computed values.
        Context.CurrentLaneSplineDistance  = Context.OwnSplineDistance;
        Context.CurrentLaneLateralOffsetCm = Context.OwnLateralOffsetCm;
    }

    // Evaluate whether a lane change is warranted (reaction-tick rate).
    EvaluateLaneChange();
}

void ARacingAIController::EvaluateState()
{
    if (!RacerData) { return; }

    const FRacingAIConfig& Cfg = RacerData->AIConfig;

    // Ensure Racing is always in the set as a baseline
    ActiveStates.Add(ERacingAIState::Racing);

    // --- Aggression behavior rules ---
    for (int32 i = 0; i < Cfg.BehaviorRules.Num(); ++i)
    {
        const FAggressionBehaviorRule& Rule = Cfg.BehaviorRules[i];

        // Skip if on cooldown
        if (BehaviorRuleCooldowns[i] > 0.0f) { continue; }

        // Skip if condition not met
        if (!CheckCondition(Rule.Condition, Rule.DistanceThreshold,
                            Rule.SpeedThreshold, 0.0f)) { continue; }

        // Probability roll (scaled by global aggression)
        const float EffectiveProbability = FMath::Clamp(
            Rule.Probability * EffectiveAggressionScale, 0.0f, 1.0f);

        if (FMath::FRand() <= EffectiveProbability)
        {
            ActiveStates.Add(Rule.Behavior);
            StateDurationRemaining.FindOrAdd(Rule.Behavior) = Rule.DurationSeconds;
            BehaviorRuleCooldowns[i] = Rule.DurationSeconds + Rule.CooldownSeconds;
        }
    }

    // --- Nitro rules ---
    for (int32 i = 0; i < Cfg.NitroRules.Num(); ++i)
    {
        const FNitroUsageRule& Rule = Cfg.NitroRules[i];

        if (NitroRuleCooldowns[i] > 0.0f) { continue; }

        if (!CheckCondition(Rule.Condition, Rule.DistanceThreshold,
                            0.0f, Rule.NitroThreshold)) { continue; }

        // No nitro left � skip
        if (Context.OwnNitroFraction <= 0.0f) { continue; }

        const float EffectiveProbability = FMath::Clamp(
            Rule.Probability * EffectiveAggressionScale, 0.0f, 1.0f);

        if (FMath::FRand() <= EffectiveProbability)
        {
            ActiveStates.Add(ERacingAIState::UsingNitro);
            NitroRuleCooldowns[i] = Rule.CooldownSeconds;
        }
    }
}

bool ARacingAIController::CheckCondition(EAIBehaviorCondition Condition,
                                          float DistanceThreshold,
                                          float SpeedThreshold,
                                          float NitroThreshold) const
{
    const float AbsDist = FMath::Abs(Context.DistanceToPlayerCm);

    switch (Condition)
    {
        case EAIBehaviorCondition::Always:
            return true;

        case EAIBehaviorCondition::Trailing:
            return Context.bPlayerAhead && AbsDist >= DistanceThreshold;

        case EAIBehaviorCondition::Leading:
            return !Context.bPlayerAhead && AbsDist >= DistanceThreshold;

        case EAIBehaviorCondition::PlayerClose:
            return !Context.bPlayerAhead && AbsDist <= DistanceThreshold;

        case EAIBehaviorCondition::PlayerCloseAhead:
            return Context.bPlayerAhead && AbsDist <= DistanceThreshold;

        case EAIBehaviorCondition::NitroAvailable:
            return Context.OwnNitroFraction >= NitroThreshold;

        case EAIBehaviorCondition::NitroLow:
            return Context.OwnNitroFraction < NitroThreshold;

        case EAIBehaviorCondition::SpeedLow:
            return Context.OwnSpeedCmS < SpeedThreshold;
    }

    return false;
}

void ARacingAIController::ExecuteState()
{
    if (!OwnPawn || !RacerData) { return; }

    // --- Idle patrol mode: bypass race state machine entirely ---
    if (bIdleActive && ActiveStates.Contains(ERacingAIState::Idle))
    {
        Execute_Idle();
        return;
    }

    float Throttle = 1.0f;
    float Steering = 0.0f;
    float Brake    = 0.0f;

    // --- Primary state execution ---
    // Blocking and bumping override the default racing line steering
    if (ActiveStates.Contains(ERacingAIState::Bumping))
    {
        // Find the rule that activated bumping to get its intensity
        const FRacingAIConfig& Cfg = RacerData->AIConfig;
        for (const FAggressionBehaviorRule& Rule : Cfg.BehaviorRules)
        {
            if (Rule.Behavior == ERacingAIState::Bumping)
            {
                Execute_Bumping(Rule);
                Throttle = 1.0f - Rule.ThrottleReduction;
                Steering = ComputeLateralToPlayer() * Rule.SteeringIntensity;
                break;
            }
        }
    }
    else if (ActiveStates.Contains(ERacingAIState::BlockingMirror))
    {
        Execute_BlockingMirror();
        Steering = Context.PlayerLateralOffsetCm > 0.0f ? 1.0f : -1.0f;
        Steering *= 0.5f; // gentle mirror
    }
    else if (ActiveStates.Contains(ERacingAIState::BlockingSlowDrift))
    {
        const FRacingAIConfig& Cfg = RacerData->AIConfig;
        for (const FAggressionBehaviorRule& Rule : Cfg.BehaviorRules)
        {
            if (Rule.Behavior == ERacingAIState::BlockingSlowDrift)
            {
                Execute_BlockingSlowDrift(Rule);
                Throttle = 1.0f - Rule.ThrottleReduction;
                Steering = ComputeLateralToPlayer() * Rule.SteeringIntensity;
                break;
            }
        }
    }
    else
    {
        // Default racing line follow
        Execute_Racing();
        Steering = ComputeSplineSteeringInput();
    }

    // --- Cornering modifier ---
    if (ActiveStates.Contains(ERacingAIState::Cornering) || Context.bCornerAhead)
    {
        ApplyCorneringModifier(Throttle, Steering, Brake);
    }

    // --- Rubber-band throttle boost ---
    ApplyRubberBand(Throttle);

    SetThrottle(FMath::Clamp(Throttle, 0.0f, 1.0f));
    SetSteering(FMath::Clamp(Steering, -1.0f, 1.0f));
    SetBrake(FMath::Clamp(Brake, 0.0f, 1.0f));

    // --- Nitro activation ---
    if (ActiveStates.Contains(ERacingAIState::UsingNitro))
    {
        Execute_Nitro();
    }
}

// ---------------------------------------------------------------------------
// State execution helpers
// ---------------------------------------------------------------------------

void ARacingAIController::Execute_Racing()
{
    // Baseline: full throttle, steer toward spline � handled in ExecuteState
}

void ARacingAIController::Execute_Idle()
{
    if (!OwnPawn) { return; }

    // Drive along the current lane spline (may differ from PatrolSpline if a
    // lane change has occurred).  Fall back to PatrolSpline then RacingSpline.
    URacingSplineComponent* Spline = CurrentLaneSpline
                                   ? CurrentLaneSpline
                                   : (PatrolSpline ? PatrolSpline : RacingSpline);
    if (!Spline) { return; }

    // Chaos puts vehicle bodies to sleep when stationary.
    // Wake every frame so throttle input is always processed.
    if (USkeletalMeshComponent* Mesh = OwnPawn->GetMesh())
    {
        Mesh->WakeAllRigidBodies();
    }

    const float TargetSpeedCmS = (RacerData ? RacerData->AIConfig.IdleTargetSpeedMPH : 35.0f)
                                 * 44.704f; // MPH -> cm/s
    const float FollowStrength = RacerData
        ? RacerData->AIConfig.IdleSplineFollowStrength
        : 0.65f;

    const float SplineLen  = Spline->GetSplineLength();
    const FVector MyPos    = OwnPawn->GetActorLocation();
    const float  NearDist  = Spline->GetNearestSplineDistance(MyPos);
    const float  TargetDist = FMath::Fmod(NearDist + 800.f, SplineLen);
    const FVector TargetPos = Spline->GetLocationAtDistance(TargetDist);

    const FVector ToTarget   = (TargetPos - MyPos).GetSafeNormal();
    const FVector RightVec   = OwnPawn->GetActorRightVector();\

    // Proportional speed controller: ramp throttle linearly from 0 at target
    // speed down to full throttle when 500 cm/s (~11 MPH) below target.
    // Naturally adds throttle on uphills and lifts off on downhills.
    const float CurrentSpeedCmS = OwnPawn->GetChaosVehicleMovement()->GetForwardSpeed();
    const float SpeedError       = TargetSpeedCmS - CurrentSpeedCmS;
    const float ProportionalGain = 500.0f; // cm/s error that maps to full throttle

    float Throttle = FMath::Clamp(SpeedError / ProportionalGain, 0.f, 1.f);
    float Steering = FMath::Clamp(
        FVector::DotProduct(ToTarget, RightVec) * FollowStrength, -1.f, 1.f);
    float Brake    = 0.f;

    // Apply corner braking so idle NPCs slow down for turns.
    if (Context.bCornerAhead)
    {
        ApplyCorneringModifier(Throttle, Steering, Brake);
    }

    SetThrottle(FMath::Clamp(Throttle, 0.f, 1.f));
    SetSteering(FMath::Clamp(Steering, -1.f, 1.f));
    SetBrake(FMath::Clamp(Brake, 0.f, 1.f));
}

void ARacingAIController::Execute_BlockingMirror()
{
    // Steering value computed in ExecuteState using player lateral offset
}

void ARacingAIController::Execute_BlockingSlowDrift(const FAggressionBehaviorRule& /*Rule*/)
{
    // Throttle and steering computed in ExecuteState from rule parameters
}

void ARacingAIController::Execute_Bumping(const FAggressionBehaviorRule& /*Rule*/)
{
    // Steering toward player computed in ExecuteState
}

void ARacingAIController::Execute_Nitro()
{
    // Trigger nitro on the pawn � wire to actual nitro activation interface
    // when that system is exposed on AVehicleExamplePawn
    // e.g. OwnPawn->ActivateNitro();
}

void ARacingAIController::ApplyCorneringModifier(float& OutThrottle, float& OutSteering, float& OutBrake) const
{
    if (!RacerData) { return; }

    const FCorneringConfig& Cfg = RacerData->AIConfig.CorneringConfig;

    if (Context.UpcomingCurvature > SMALL_NUMBER)
    {
        // Physics: max safe speed = sqrt(lateral_accel / curvature)
        // curvature is in rad/cm so radius = 1/curvature in cm
        const float PhysicsMaxSpeed = FMath::Sqrt(
            Cfg.AILateralAccelCmS2 / Context.UpcomingCurvature);

        // Optional designer cap � use the lower of physics and cap (if cap is set)
        const float MaxSpeedCmS = (Cfg.MaxCornerSpeedCmS > 0.0f)
            ? FMath::Min(PhysicsMaxSpeed, Cfg.MaxCornerSpeedCmS)
            : PhysicsMaxSpeed;

        if (Context.OwnSpeedCmS > MaxSpeedCmS)
        {
            OutThrottle = 0.0f;

            // Brake proportional to how far over the limit we are.
            // At 2x the limit ? full brake; scales linearly.
            const float OverFraction = (Context.OwnSpeedCmS - MaxSpeedCmS)
                                     / FMath::Max(MaxSpeedCmS, 1.0f);
            OutBrake = FMath::Clamp(OverFraction, 0.0f, 1.0f);
        }
    }

    // Blend steering toward the spline tangent
    const float SplineSteer = ComputeSplineSteeringInput();
    OutSteering = FMath::Lerp(OutSteering, SplineSteer, Cfg.SplineFollowStrength);
}

void ARacingAIController::ApplyRubberBand(float& OutThrottle) const
{
    if (!RacerData || !Context.bPlayerAhead) { return; }

    const FRacingAIConfig& Cfg = RacerData->AIConfig;
    if (Cfg.RubberBandActivationDistanceCm <= 0.0f) { return; }

    const float Gap = Context.DistanceToPlayerCm;
    if (Gap < Cfg.RubberBandActivationDistanceCm) { return; }

    // Linear ramp from 0 at activation distance to full bonus at 2x activation distance
    const float Alpha = FMath::Clamp(
        (Gap - Cfg.RubberBandActivationDistanceCm) / Cfg.RubberBandActivationDistanceCm,
        0.0f, 1.0f);

    OutThrottle = FMath::Min(1.0f, OutThrottle + EffectiveRubberBandBonus * Alpha);
}

// ---------------------------------------------------------------------------
// Pawn input helpers
// ---------------------------------------------------------------------------

void ARacingAIController::SetThrottle(float Value)
{
    if (OwnPawn) { OwnPawn->DoThrottle(Value); }
}

void ARacingAIController::SetSteering(float Value)
{
    if (OwnPawn) { OwnPawn->DoSteering(Value); }
}

void ARacingAIController::SetBrake(float Value)
{
    // Do NOT use DoBrake � it unconditionally resets throttle to 0,
    // which is correct for player input but wrong for AI.
    if (OwnPawn && OwnPawn->GetChaosVehicleMovement())
    {
        OwnPawn->GetChaosVehicleMovement()->SetBrakeInput(Value);
    }
}

float ARacingAIController::ComputeSplineSteeringInput() const
{
    if (!OwnPawn) { return 0.0f; }

    const FVector PawnForward = OwnPawn->GetActorForwardVector();

    const FCorneringConfig& Cfg = RacerData
        ? RacerData->AIConfig.CorneringConfig
        : FCorneringConfig{};

    // Helper lambda: compute steering for a given spline and distance along it.
    // Uses the two-component approach: heading (lookahead tangent) + gentle
    // lateral correction toward lane centre.
    auto SteerForLane = [&](URacingSplineComponent* Spline,
                             float LaneDistCm,
                             float LaneLateralOffsetCm) -> float
    {
        if (!Spline) { return 0.0f; }

        // Component 1: heading — sample the tangent SteeringLookaheadCm ahead
        // on this lane so the car turns in early for corners.
        const FVector LookaheadTangent = Spline->GetDirectionAtDistance(
            LaneDistCm + Cfg.SteeringLookaheadCm);
        const float HeadingSteering = FVector::CrossProduct(PawnForward, LookaheadTangent).Z;

        // Component 2: low-gain lateral correction toward the lane centre.
        const float LateralSteering = FMath::Clamp(
            LaneLateralOffsetCm / FMath::Max(Cfg.LateralCorrectionScaleCm, 1.0f),
            -1.0f, 1.0f);

        return FMath::Clamp(
            HeadingSteering + Cfg.LateralCorrectionWeight * LateralSteering,
            -1.0f, 1.0f);
    };

    // Use CurrentLaneSpline (the lane the NPC is committed to).
    // Fall back to RacingSpline for backwards compatibility if lane data absent.
    URacingSplineComponent* ActiveLane = CurrentLaneSpline ? CurrentLaneSpline : RacingSpline;
    if (!ActiveLane)
    {
        return FMath::Clamp(FVector::CrossProduct(PawnForward, Context.SplineTangent).Z, -1.0f, 1.0f);
    }

    const float CurrentResult = SteerForLane(
        ActiveLane,
        Context.CurrentLaneSplineDistance,
        Context.CurrentLaneLateralOffsetCm);

    // During a lane-change blend, interpolate from the previous lane's steering
    // toward the new lane's steering.  This prevents a sudden snap in heading
    // as the AI transitions between splines.
    if (PreviousLaneSpline && LaneBlendAlpha < 1.0f)
    {
        // Compute previous-lane distance by projecting own position onto it.
        const float PrevLaneDist   = PreviousLaneSpline->GetNearestSplineDistance(
            OwnPawn->GetActorLocation());
        const FVector PrevLanePos  = PreviousLaneSpline->GetLocationAtDistance(PrevLaneDist);
        const FVector PrevTangent  = PreviousLaneSpline->GetDirectionAtDistance(PrevLaneDist);
        const FVector PrevRight    = FVector::CrossProduct(PrevTangent, FVector::UpVector);
        const float   PrevLateral  = FVector::DotProduct(
            OwnPawn->GetActorLocation() - PrevLanePos, PrevRight);

        const float PreviousResult = SteerForLane(PreviousLaneSpline, PrevLaneDist, PrevLateral);
        return FMath::Lerp(PreviousResult, CurrentResult, LaneBlendAlpha);
    }

    return CurrentResult;
}

float ARacingAIController::ComputeLateralToPlayer() const
{
    if (!OwnPawn || !PlayerPawn) { return 0.0f; }

    // Project player position into pawn's local right axis
    const FVector ToPlayer = PlayerPawn->GetActorLocation() - OwnPawn->GetActorLocation();
    const FVector Right    = OwnPawn->GetActorRightVector();
    return FMath::Clamp(FVector::DotProduct(ToPlayer.GetSafeNormal(), Right), -1.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Lane driving
// ---------------------------------------------------------------------------

void ARacingAIController::TickLaneBlend(float DeltaSeconds)
{
    if (!PreviousLaneSpline) { return; }

    const float Duration = RacerData
        ? RacerData->AIConfig.LaneConfig.LaneChangeDurationSec
        : 1.5f;

    LaneBlendAlpha += DeltaSeconds / FMath::Max(Duration, KINDA_SMALL_NUMBER);
    if (LaneBlendAlpha >= 1.0f)
    {
        LaneBlendAlpha     = 1.0f;
        PreviousLaneSpline = nullptr;
    }
}

void ARacingAIController::EvaluateLaneChange()
{
    // Skip if we are already mid-transition — wait until the blend completes.
    if (PreviousLaneSpline) { return; }
    if (!OwnPawn || !CurrentLaneSpline) { return; }

    const FLaneConfig& LaneCfg = RacerData
        ? RacerData->AIConfig.LaneConfig
        : FLaneConfig{};

    const FVector OwnLocation  = OwnPawn->GetActorLocation();
    const FVector OwnForward   = OwnPawn->GetActorForwardVector();
    const float   SweepRadius  = LaneCfg.ObstacleSweepRadiusCm;
    const float   LookaheadCm  = LaneCfg.ObstacleLookaheadCm;

    // Build the sweep: a sphere trace along the NPC's forward direction.
    const FVector SweepStart = OwnLocation;
    const FVector SweepEnd   = OwnLocation + OwnForward * LookaheadCm;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnPawn);

    TArray<FHitResult> Hits;
    const bool bAnyHit = GetWorld()->SweepMultiByChannel(
        Hits, SweepStart, SweepEnd,
        FQuat::Identity,
        ECollisionChannel::ECC_Pawn,
        FCollisionShape::MakeSphere(SweepRadius),
        QueryParams);

    if (!bAnyHit) { return; }

    // Check whether any hit is a vehicle ahead of us on the current lane.
    bool bLaneBlocked = false;
    for (const FHitResult& Hit : Hits)
    {
        if (!Hit.GetActor()) { continue; }
        if (!Hit.GetActor()->IsA<AVehicleExamplePawn>()) { continue; }

        // Only consider vehicles that are ahead (not ones we just passed).
        const FVector ToHit = (Hit.ImpactPoint - OwnLocation);
        if (FVector::DotProduct(ToHit, OwnForward) > 0.0f)
        {
            bLaneBlocked = true;
            break;
        }
    }

    if (!bLaneBlocked) { return; }

    // Find the parent CourseSplineActor to get sibling lanes.
    ACourseSplineActor* LaneActor = Cast<ACourseSplineActor>(CurrentLaneSpline->GetOwner());
    if (!LaneActor) { return; }

    TArray<URacingSplineComponent*> AllLanes = LaneActor->GetAllSplines();

    // Score each sibling: count how many vehicles are in its path.
    URacingSplineComponent* BestLane       = nullptr;
    int32                    BestHitCount   = INT_MAX;
    int32                    BestLaneIndex  = -1;

    for (URacingSplineComponent* Candidate : AllLanes)
    {
        if (!Candidate || Candidate == CurrentLaneSpline) { continue; }

        // Project NPC's position onto the candidate lane to get its centre.
        const float   CandDist     = Candidate->GetNearestSplineDistance(OwnLocation);
        const FVector CandPos      = Candidate->GetLocationAtDistance(CandDist);
        const FVector CandForward  = Candidate->GetDirectionAtDistance(CandDist);

        // Sweep along candidate lane forward from its equivalent position.
        const FVector CandStart = CandPos;
        const FVector CandEnd   = CandPos + CandForward * LookaheadCm;

        TArray<FHitResult> CandHits;
        GetWorld()->SweepMultiByChannel(
            CandHits, CandStart, CandEnd,
            FQuat::Identity,
            ECollisionChannel::ECC_Pawn,
            FCollisionShape::MakeSphere(SweepRadius),
            QueryParams);

        // Count forward obstacles only.
        int32 HitCount = 0;
        for (const FHitResult& H : CandHits)
        {
            if (!H.GetActor() || !H.GetActor()->IsA<AVehicleExamplePawn>()) { continue; }
            if (FVector::DotProduct(H.ImpactPoint - CandPos, CandForward) > 0.0f)
            {
                ++HitCount;
            }
        }

        // Prefer fewer obstacles; tiebreak on higher LaneIndex (rightmost lane).
        if (HitCount < BestHitCount ||
            (HitCount == BestHitCount && Candidate->LaneIndex > BestLaneIndex))
        {
            BestHitCount  = HitCount;
            BestLane      = Candidate;
            BestLaneIndex = Candidate->LaneIndex;
        }
    }

    // Only switch if the best candidate is actually clearer than staying put.
    if (BestLane && BestHitCount < 1)
    {
        PreviousLaneSpline = CurrentLaneSpline;
        CurrentLaneSpline  = BestLane;
        LaneBlendAlpha     = 0.0f;
    }
}
