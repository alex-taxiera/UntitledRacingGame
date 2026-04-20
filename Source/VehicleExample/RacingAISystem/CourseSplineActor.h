// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CourseSplineActor.generated.h"

class URacingSplineComponent;

/**
 * ACourseSplineActor
 *
 * Place one of these in the level and add URacingSplineComponent instances
 * to it using the Components panel (+Add button).  Name each component
 * whatever you like (e.g. "MainLoop", "ForkMountain").
 * Draw the spline points along your road mesh.  Enable Closed Loop on each.
 *
 * At runtime the spawn manager calls GetSplineByName() or GetRandomSpline()
 * to assign a patrol path to each NPC.  No configuration in C++ needed �
 * just add and draw components in the editor.
 */
UCLASS(BlueprintType, Blueprintable)
class VEHICLEEXAMPLE_API ACourseSplineActor : public AActor
{
    GENERATED_BODY()

public:

    ACourseSplineActor();

    /**
     * Returns the URacingSplineComponent whose name matches Name, or nullptr.
     * Searches all components attached to this actor � no pre-registration needed.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Course Splines")
    URacingSplineComponent* GetSplineByName(FName Name) const;

    /** Returns the names of all URacingSplineComponents on this actor. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Course Splines")
    TArray<FName> GetAllSplineNames() const;

    /** Returns a random URacingSplineComponent, or nullptr if none exist. */
    UFUNCTION(BlueprintCallable, Category = "Course Splines")
    URacingSplineComponent* GetRandomSpline() const;

    /**
     * Returns all URacingSplineComponents on this actor, sorted ascending by
     * LaneIndex.  The sorted order lets callers treat adjacent elements as
     * neighbouring lanes without any spatial proximity calculation.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Course Splines")
    TArray<URacingSplineComponent*> GetAllSplines() const;
};
