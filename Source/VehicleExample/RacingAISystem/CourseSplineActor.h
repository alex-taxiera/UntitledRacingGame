// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CourseSplineActor.generated.h"

class URacingSplineComponent;

/**
 * ACourseSplineActor
 *
 * A single actor placed in the course level that owns all named racing
 * splines for the track.  Each spline component represents one path
 * segment or loop (e.g. "MainLoop", "ForkMountain", "ForkCity").
 *
 * How to use in the editor:
 *   1. Place one ACourseSplineActor (or a Blueprint subclass) in the level.
 *   2. In the Details panel, expand "Course Splines" and click the +
 *      button on the SplineNames array to add a name (e.g. "MainLoop").
 *   3. BeginPlay will automatically create one URacingSplineComponent per
 *      entry in SplineNames.  Each component appears in the Components panel.
 *   4. Select each component and draw the spline by dragging its points
 *      along your road mesh.  Enable bClosedLoop on each one.
 *   5. ACourseGameMode calls GetSplineByName() to hand the right spline
 *      to each NPC patrol actor.
 *
 * You can also call AddSplineForName() at runtime (e.g. from a Blueprint)
 * to register procedurally-created splines.
 */
UCLASS(BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API ACourseSplineActor : public AActor
{
    GENERATED_BODY()

public:

    ACourseSplineActor();

    /**
     * Called in-editor whenever a property changes (including SplineNames).
     * Creates one URacingSplineComponent per entry in SplineNames so they
     * are immediately visible and editable in the Components panel.
     */
    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void BeginPlay() override;

    // -----------------------------------------------------------------------
    // Designer-facing spline list
    // -----------------------------------------------------------------------

    /**
     * Names of the patrol paths on this course.
     * Add an entry here and a URacingSplineComponent is immediately created
     * in the editor's Components panel, ready for you to draw spline points.
     *
     * Example entries: "MainLoop", "ForkMountain", "ForkCity"
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course Splines")
    TArray<FName> SplineNames;

    // -----------------------------------------------------------------------
    // Runtime queries
    // -----------------------------------------------------------------------

    /**
     * Returns the URacingSplineComponent registered under Name, or nullptr.
     * Case-sensitive match against SplineNames.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Course Splines")
    URacingSplineComponent* GetSplineByName(FName Name) const;

    /**
     * Returns all registered spline names.
     * Used by the spawn manager to assign splines to NPCs whose
     * PatrolSplineName is empty.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Course Splines")
    TArray<FName> GetAllSplineNames() const;

    /**
     * Returns a random registered spline.
     * Used as a fallback when an NPC's preferred spline is unavailable.
     * Returns nullptr if no splines are registered.
     */
    UFUNCTION(BlueprintCallable, Category = "Course Splines")
    URacingSplineComponent* GetRandomSpline() const;

    /**
     * Manually registers an externally-created spline under the given name.
     * No-op if a spline is already registered under that name.
     */
    UFUNCTION(BlueprintCallable, Category = "Course Splines")
    void AddSplineForName(FName Name, URacingSplineComponent* Spline);

private:

    /** Runtime map from name to component. Populated in BeginPlay. */
    UPROPERTY()
    TMap<FName, TObjectPtr<URacingSplineComponent>> Splines;
};
