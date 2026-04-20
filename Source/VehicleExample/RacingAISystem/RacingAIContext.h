// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RacingAITypes.h"
#include "RacingAIContext.generated.h"

class URacingSplineComponent;
class AVehicleExamplePawn;

/**
 * FRacingAIContext
 *
 * A plain data struct holding all per-tick sense data for one NPC racer.
 * Rebuilt every reaction tick by ARacingAIController::UpdateContext().
 * Passed by const-ref to all evaluation and execution functions so they
 * never need to query the world directly.
 *
 * Non-owning pointers (OwnPawn, PlayerPawn, RacingSpline) are valid only
 * for the duration of the tick � do not cache this struct across frames.
 */
USTRUCT(BlueprintType)
struct FRacingAIContext
{
    GENERATED_BODY()

    // -----------------------------------------------------------------------
    // Positional / distance
    // -----------------------------------------------------------------------

    /**
     * Signed distance along the racing spline between this NPC and the player.
     * Positive = player is ahead; negative = player is behind.
     * Units: cm.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float DistanceToPlayerCm = 0.0f;

    /** True if the player is ahead of this NPC on the racing line. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    bool bPlayerAhead = false;

    /** Lateral offset of the player from the racing line in cm (negative = left). */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float PlayerLateralOffsetCm = 0.0f;

    // -----------------------------------------------------------------------
    // Own vehicle state
    // -----------------------------------------------------------------------

    /** Current forward speed of the NPC vehicle in cm/s. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float OwnSpeedCmS = 0.0f;

    /** Currently engaged gear (1-based forward, 0 = neutral, -1 = reverse). */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    int32 OwnGear = 0;

    /** Remaining nitro as a 0�1 fraction of max capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float OwnNitroFraction = 0.0f;

    /** Distance along the racing spline for this NPC's current position. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float OwnSplineDistance = 0.0f;

    /** Lateral offset of this NPC from the racing line in cm (negative = left). */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float OwnLateralOffsetCm = 0.0f;

    // -----------------------------------------------------------------------
    // Current lane state (relative to CurrentLaneSpline, not the ref spline)
    // -----------------------------------------------------------------------

    /**
     * Distance along CurrentLaneSpline for this NPC's current position.
     * Used by ComputeSplineSteeringInput() and Execute_Idle() to aim at a
     * lookahead point on the actual lane being driven, not the reference line.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float CurrentLaneSplineDistance = 0.0f;

    /**
     * Lateral offset from CurrentLaneSpline in cm (negative = left).
     * Used in place of OwnLateralOffsetCm for lane-centring corrections so
     * the NPC steers to the centre of its chosen lane rather than the
     * reference spline centerline.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float CurrentLaneLateralOffsetCm = 0.0f;

    // -----------------------------------------------------------------------
    // Corner awareness
    // -----------------------------------------------------------------------

    /**
     * Curvature magnitude sampled LookaheadDistance ahead of the NPC.
     * Units: radians/cm (1/radius). 0 = straight; larger = tighter corner.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float UpcomingCurvature = 0.0f;

    /** True when UpcomingCurvature exceeds the config threshold. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    bool bCornerAhead = false;

    /**
     * Forward tangent direction of the racing line at the NPC's current position.
     * Used as the steering target to re-align with the ideal line.
     */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    FVector SplineTangent = FVector::ForwardVector;

    // -----------------------------------------------------------------------
    // Timing
    // -----------------------------------------------------------------------

    /** Total elapsed race time in seconds. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float ElapsedRaceTimeSec = 0.0f;

    /** Seconds elapsed since the current primary state was entered. */
    UPROPERTY(BlueprintReadOnly, Category = "AIContext")
    float TimeInCurrentStateSec = 0.0f;

    // -----------------------------------------------------------------------
    // Non-owning pointers (valid for this tick only � do not cache)
    // -----------------------------------------------------------------------

    /** The NPC's own vehicle pawn. */
    AVehicleExamplePawn* OwnPawn = nullptr;

    /** The player's vehicle pawn. */
    AVehicleExamplePawn* PlayerPawn = nullptr;

    /** The racing-line spline for this track. May be null if no spline is set. */
    URacingSplineComponent* RacingSpline = nullptr;
};
