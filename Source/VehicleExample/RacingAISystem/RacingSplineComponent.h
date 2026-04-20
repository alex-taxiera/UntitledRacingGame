// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "RacingSplineComponent.generated.h"

/**
 * URacingSplineComponent
 *
 * A thin USplineComponent subclass that marks a spline as the canonical
 * racing line for a track section.  Attach this to an Actor in the level
 * and it will be automatically found by ARacingAIController at race start.
 *
 * The spline should follow the ideal racing line through each corner.
 * The AI controller projects its own position onto this spline each tick
 * to determine the nearest point, look ahead for corners, and steer back
 * toward the racing line.
 *
 * Usage:
 *   1. Place an Actor in the level.
 *   2. Add a URacingSplineComponent to it (or use a Blueprint subclass).
 *   3. Lay out the spline along the racing line. Enable bClosedLoop if the
 *      track is a circuit.
 *   4. Call ARacingAIController::SetRacingSpline() from your game mode,
 *      or let the controller auto-find it via FindInWorld().
 */
UCLASS(ClassGroup = "Racing", meta = (BlueprintSpawnableComponent),
       BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API URacingSplineComponent : public USplineComponent
{
    GENERATED_BODY()

public:

    URacingSplineComponent();

    // -----------------------------------------------------------------------
    // Lane grouping
    // -----------------------------------------------------------------------

    /**
     * Index of this lane within its CourseSplineActor group.
     * 0 = leftmost lane, increasing rightward relative to the travel direction.
     * Trusted as correct by the AI — no spatial validation is performed.
     * Used by EvaluateLaneChange() to tiebreak between equally clear candidates:
     * the rightmost available lane is preferred (standard road driving convention).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lane")
    int32 LaneIndex = 0;

    // -----------------------------------------------------------------------
    // Racing-line queries
    // -----------------------------------------------------------------------

    /**
     * Returns the distance along the spline of the point nearest to WorldLocation.
     * Uses binary search over spline segments for reasonable accuracy.
     *
     * @param WorldLocation  The world-space position to project onto the spline.
     * @param NumIterations  Search refinement iterations (higher = more accurate, more expensive).
     */
    UFUNCTION(BlueprintCallable, Category = "RacingSpline")
    float GetNearestSplineDistance(const FVector& WorldLocation, int32 NumIterations = 8) const;

    /**
     * Samples curvature of the spline at a point DistanceAlongSpline + LookaheadDist ahead.
     * Curvature is approximated as the angle between the tangent at two sample points
     * divided by the arc length between them (units: radians per cm, i.e. 1/radius).
     *
     * @param DistanceAlongSpline  Start distance from which to look ahead.
     * @param LookaheadDist        How far ahead (in cm) to sample curvature.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingSpline")
    float GetLookaheadCurvature(float DistanceAlongSpline, float LookaheadDist) const;

    /**
     * Scans the entire lookahead window and returns the WORST (highest) curvature
     * found at any point within it.  Use this instead of GetLookaheadCurvature
     * when you need to know the tightest corner the car will face, not just the
     * curvature at one arbitrary sample point ahead.
     *
     * @param DistanceAlongSpline  Start distance.
     * @param LookaheadDist        How far ahead (cm) to scan.
     * @param SampleStep           Distance between each curvature sample (cm).
     *                             Smaller = more accurate but more expensive.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingSpline")
    float GetMaxCurvatureInRange(float DistanceAlongSpline,
                                 float LookaheadDist,
                                 float SampleStep = 200.0f) const;

    /**
     * Returns the world-space forward tangent direction at the given spline distance.
     * Use this to align the NPC's steering input with the racing line.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingSpline")
    FVector GetDirectionAtDistance(float Distance) const;

    /**
     * Returns the world-space location on the spline at the given distance.
     */
    UFUNCTION(BlueprintCallable, Category = "RacingSpline")
    FVector GetLocationAtDistance(float Distance) const;

    // -----------------------------------------------------------------------
    // World search helper
    // -----------------------------------------------------------------------

    /**
     * Finds the first URacingSplineComponent in the given world.
     * Called by ARacingAIController::BeginPlay when no spline has been explicitly set.
     * Returns nullptr if none is found.
     */
    static URacingSplineComponent* FindInWorld(UWorld* World);
};
