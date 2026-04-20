// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "RacingAITypes.h"
#include "RacingAIContext.h"
#include "RacingAIController.generated.h"

class URacingSplineComponent;
class UNPCRacerData;
class AVehicleExamplePawn;
class UChaosWheeledVehicleMovementComponent;

/**
 * ARacingAIController
 *
 * AI controller for NPC racing vehicles.  Drives the pawn using the same
 * throttle/steering/brake input methods exposed on AVehicleExamplePawn
 * (DoThrottle, DoSteering, DoBrake) so the physics simulation is identical
 * to the player's vehicle.
 *
 * Architecture: C++ state machine
 *   UpdateContext()  � reads world state into FRacingAIContext each reaction tick
 *   EvaluateState()  � checks behavior rules, rolls probability, sets active states
 *   ExecuteState()   � converts active state set into throttle/steering/brake inputs
 *   Cornering acts as a modifier layer on top of every primary state.
 *
 * How to use:
 *   1. Set the AI Controller Class on your NPC pawn Blueprint to ARacingAIController
 *      (or a Blueprint subclass).
 *   2. In your game mode, after spawning the NPC pawn call:
 *        Controller->SetRacerData(MyNPCRacerData);
 *        Controller->SetRacingSpline(MySpline);        // optional; auto-finds if null
 *        Controller->SetPlayerPawn(PlayerPawn);
 *        Controller->ApplyDifficultyOverride(GlobalOverride);
 *   3. Call Controller->StartRace() when the race begins.
 *   4. Call Controller->EndRace() when the race ends.
 */
UCLASS(BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API ARacingAIController : public AAIController
{
    GENERATED_BODY()

public:

    ARacingAIController();

    // -----------------------------------------------------------------------
    // Setup  (called by game mode before race start)
    // -----------------------------------------------------------------------

    /**
     * Assigns the NPC racer data asset that defines this controller's behaviour.
     * Must be called before StartRace().
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void SetRacerData(UNPCRacerData* InRacerData);

    /**
     * Assigns the racing-line spline.
     * If not called explicitly, BeginPlay will search the world for one.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void SetRacingSpline(URacingSplineComponent* InSpline);

    /**
     * Assigns the player's pawn so the AI can sense relative position.
     * Must be called before StartRace().
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void SetPlayerPawn(AVehicleExamplePawn* InPlayerPawn);

    /**
     * Applies a global difficulty override on top of the NPC's own per-NPC
     * override.  Multipliers are combined: effective = global * npc-specific.
     * Call before StartRace() or at any time to adjust mid-race.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void ApplyDifficultyOverride(const FAIDifficultyOverride& GlobalOverride);

    // -----------------------------------------------------------------------
    // Race lifecycle
    // -----------------------------------------------------------------------

    /** Activates the AI. Starts the reaction timer and begins ticking. */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void StartRace();

    /** Deactivates the AI. Clears all inputs on the pawn and stops the timer. */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void EndRace();

    /**
     * Puts the NPC into idle patrol mode.
     * The NPC follows PatrolSpline (falls back to RacingSpline) at IdleThrottle.
     * Does nothing if a race is already active.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void StartIdle();

    /**
     * Stops idle patrol without starting a race.
     * Call before StartRace() when transitioning from patrol to battle.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void StopIdle();

    /**
     * Sets the spline used specifically for idle patrol.
     * If null the controller falls back to RacingSpline for patrol too.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingAI")
    void SetPatrolSpline(URacingSplineComponent* InSpline);

    // -----------------------------------------------------------------------
    // State inspection (for debug / HUD)
    // -----------------------------------------------------------------------

    /** Returns the set of states currently active on this NPC. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RacingAI")
    TSet<ERacingAIState> GetActiveStates() const { return ActiveStates; }

    /** Returns true if the given state is currently active. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RacingAI")
    bool IsStateActive(ERacingAIState State) const { return ActiveStates.Contains(State); }

    bool IsIdleActive()  const { return bIdleActive; }
    bool IsRaceActive()  const { return bRaceActive; }
    AVehicleExamplePawn*    GetOwnPawn()       const { return OwnPawn; }
    UNPCRacerData*          GetRacerData()     const { return RacerData; }
    URacingSplineComponent* GetPatrolSpline()  const { return PatrolSpline; }
    URacingSplineComponent* GetRacingSpline()  const { return RacingSpline; }

    /** Returns the most recently computed AI context (read-only snapshot). */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RacingAI")
    const FRacingAIContext& GetContext() const { return Context; }

    // -----------------------------------------------------------------------
    // AActor / AController interface
    // -----------------------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaSeconds) override;

private:

    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UNPCRacerData> RacerData;

    UPROPERTY()
    TObjectPtr<URacingSplineComponent> RacingSpline;

    /** Spline used during idle patrol. Falls back to RacingSpline if null. */
    UPROPERTY()
    TObjectPtr<URacingSplineComponent> PatrolSpline;

    // -----------------------------------------------------------------------
    // Lane driving state
    // -----------------------------------------------------------------------

    /**
     * The lane spline the NPC is currently driving along.
     * Initialized to PatrolSpline (or RacingSpline) in StartIdle()/StartRace().
     * EvaluateLaneChange() may swap this to a sibling spline on the same
     * CourseSplineActor when an obstacle is detected ahead.
     */
    UPROPERTY()
    TObjectPtr<URacingSplineComponent> CurrentLaneSpline;

    /**
     * The lane spline the NPC was driving before the most recent lane change.
     * Non-null only while a blend transition is in progress.
     * Cleared when LaneBlendAlpha reaches 1.0.
     */
    UPROPERTY()
    TObjectPtr<URacingSplineComponent> PreviousLaneSpline;

    /**
     * 0-to-1 progress of the current lane-change blend.
     * 0 = steering fully on PreviousLaneSpline; 1 = fully on CurrentLaneSpline.
     * Advanced every frame by TickLaneBlend().
     */
    float LaneBlendAlpha = 1.0f;

    UPROPERTY()
    TObjectPtr<AVehicleExamplePawn> PlayerPawn;

    UPROPERTY()
    TObjectPtr<AVehicleExamplePawn> OwnPawn;

    /** Active state set � multiple states can be simultaneously active. */
    TSet<ERacingAIState> ActiveStates;

    /** Per-state duration timers (seconds remaining while the state is active). */
    TMap<ERacingAIState, float> StateDurationRemaining;

    /** Per-rule cooldown trackers for aggression behavior rules (index = rule index). */
    TArray<float> BehaviorRuleCooldowns;

    /** Per-rule cooldown trackers for nitro usage rules (index = rule index). */
    TArray<float> NitroRuleCooldowns;

    /** Most recently computed context (updated each reaction tick). */
    FRacingAIContext Context;

    /** Seconds since the last reaction tick. */
    float TimeSinceLastReaction = 0.0f;

    /** Effective reaction time after difficulty scaling. */
    float EffectiveReactionTime = 0.25f;

    /** Effective aggression scale after difficulty scaling (0�1). */
    float EffectiveAggressionScale = 0.5f;

    /** Effective rubber-band throttle bonus scale after difficulty scaling. */
    float EffectiveRubberBandBonus = 0.0f;

    /** Seconds spent in the current primary state (used by context). */
    float TimeInCurrentState = 0.0f;

    bool bRaceActive = false;
    bool bIdleActive = false;

    // -----------------------------------------------------------------------
    // Core state machine
    // -----------------------------------------------------------------------

    /** Rebuilds FRacingAIContext from current world state. */
    void UpdateContext();

    /** Evaluates all behavior and nitro rules, updates ActiveStates. */
    void EvaluateState();

    /** Dispatches execution of all currently active states. */
    void ExecuteState();

    /** Called at EffectiveReactionTime intervals via Tick. */
    void ReactionTick(float DeltaSeconds);

    // -----------------------------------------------------------------------
    // Condition check helpers
    // -----------------------------------------------------------------------

    bool CheckCondition(EAIBehaviorCondition Condition,
                        float DistanceThreshold,
                        float SpeedThreshold,
                        float NitroThreshold) const;

    // -----------------------------------------------------------------------
    // State execution helpers
    // -----------------------------------------------------------------------

    void Execute_Racing();
    void Execute_Idle();
    void Execute_BlockingMirror();
    void Execute_BlockingSlowDrift(const FAggressionBehaviorRule& Rule);
    void Execute_Bumping(const FAggressionBehaviorRule& Rule);
    void Execute_Nitro();

    /**
     * Advances LaneBlendAlpha toward 1.0 each frame.
     * Clears PreviousLaneSpline once the transition completes.
     * Called from Tick() before ExecuteState() so steering inputs always
     * use an up-to-date blend weight.
     */
    void TickLaneBlend(float DeltaSeconds);

    /**
     * Checks whether any AVehicleExamplePawn is blocking the NPC's current
     * lane ahead and, if so, picks the clearest sibling lane from the same
     * CourseSplineActor and begins a lane-change transition.
     * Called at reaction-tick rate from UpdateContext().
     */
    void EvaluateLaneChange();

    /**
     * Reduces throttle and applies brakes proportionally when the NPC is
     * over the configured corner speed limit.  Also blends steering toward
     * the spline tangent.  OutBrake is additive � call site should clamp 0-1.
     */
    void ApplyCorneringModifier(float& OutThrottle, float& OutSteering, float& OutBrake) const;
    void ApplyRubberBand(float& OutThrottle) const;

    // -----------------------------------------------------------------------
    // Pawn input helpers
    // -----------------------------------------------------------------------

    void SetThrottle(float Value);
    void SetSteering(float Value);
    void SetBrake(float Value);

    /** Computes the steering angle needed to align with the spline tangent. */
    float ComputeSplineSteeringInput() const;

    /** Computes signed lateral deviation from the player (positive = player is to the right). */
    float ComputeLateralToPlayer() const;
};
