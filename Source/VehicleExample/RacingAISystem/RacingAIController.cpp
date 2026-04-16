// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingAIController.h"
#include "RacingSplineComponent.h"
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
// Tick — gates on reaction time
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

    // Nitro fraction — read from OwnedVehicle via pawn if available
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
                              : 2000.0f;
        Context.UpcomingCurvature = RacingSpline->GetLookaheadCurvature(
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

        // No nitro left — skip
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

    // --- Cornering modifier (overrides throttle and blends steering toward spline) ---
    if (ActiveStates.Contains(ERacingAIState::Cornering) || Context.bCornerAhead)
    {
        ApplyCorneringModifier(Throttle, Steering);
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
    // Baseline: full throttle, steer toward spline — handled in ExecuteState
}

void ARacingAIController::Execute_Idle()
{
    if (!OwnPawn) { return; }

    URacingSplineComponent* Spline = PatrolSpline ? PatrolSpline : RacingSpline;
    if (!Spline) { return; }

    const float IdleThrottle = RacerData
        ? RacerData->AIConfig.IdleThrottle
        : 0.35f;
    const float FollowStrength = RacerData
        ? RacerData->AIConfig.IdleSplineFollowStrength
        : 0.65f;

    const float SplineLen  = Spline->GetSplineLength();
    const FVector MyPos    = OwnPawn->GetActorLocation();
    const float  NearDist  = Spline->GetNearestSplineDistance(MyPos);
    // Look ahead a fixed 800 cm for the follow target
    const float  TargetDist = FMath::Fmod(NearDist + 800.f, SplineLen);
    const FVector TargetPos = Spline->GetLocationAtDistance(TargetDist);

    // Steering: signed angle to target in actor-local space
    const FVector ToTarget   = (TargetPos - MyPos).GetSafeNormal();
    const FVector RightVec   = OwnPawn->GetActorRightVector();
    const float   Lateral    = FVector::DotProduct(ToTarget, RightVec);
    const float   Steering   = FMath::Clamp(Lateral * FollowStrength, -1.f, 1.f);

    SetThrottle(IdleThrottle);
    SetSteering(Steering);
    SetBrake(0.f);
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
    // Trigger nitro on the pawn — wire to actual nitro activation interface
    // when that system is exposed on AVehicleExamplePawn
    // e.g. OwnPawn->ActivateNitro();
}

void ARacingAIController::ApplyCorneringModifier(float& OutThrottle, float& OutSteering) const
{
    if (!RacerData) { return; }

    const FCorneringConfig& Cfg = RacerData->AIConfig.CorneringConfig;

    // Reduce throttle when speed exceeds the configured corner limit
    if (Cfg.MaxCornerSpeedCmS > 0.0f && Context.OwnSpeedCmS > Cfg.MaxCornerSpeedCmS)
    {
        const float SpeedExcess = (Context.OwnSpeedCmS - Cfg.MaxCornerSpeedCmS)
                                / FMath::Max(Context.OwnSpeedCmS, 1.0f);
        OutThrottle = FMath::Max(0.0f, OutThrottle - SpeedExcess);
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
    if (OwnPawn) { OwnPawn->DoBrake(Value); }
}

float ARacingAIController::ComputeSplineSteeringInput() const
{
    if (!OwnPawn) { return 0.0f; }

    // Cross product of pawn forward and spline tangent gives the signed lateral error
    const FVector PawnForward = OwnPawn->GetActorForwardVector();
    const FVector Cross       = FVector::CrossProduct(PawnForward, Context.SplineTangent);

    // Z component = signed turn direction (positive = turn right)
    return FMath::Clamp(Cross.Z, -1.0f, 1.0f);
}

float ARacingAIController::ComputeLateralToPlayer() const
{
    if (!OwnPawn || !PlayerPawn) { return 0.0f; }

    // Project player position into pawn's local right axis
    const FVector ToPlayer = PlayerPawn->GetActorLocation() - OwnPawn->GetActorLocation();
    const FVector Right    = OwnPawn->GetActorRightVector();
    return FMath::Clamp(FVector::DotProduct(ToPlayer.GetSafeNormal(), Right), -1.0f, 1.0f);
}
