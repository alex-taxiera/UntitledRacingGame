// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HubTypes.generated.h"

/**
 * EHubPanel
 *
 * Identifies each navigable panel in the hub level.
 * Used by SHubRootWidget to switch the active panel and by
 * SHubMenuOverlay to label and enable/disable navigation buttons.
 */
UENUM(BlueprintType)
enum class EHubPanel : uint8
{
    /** Vehicle dealership — browse and purchase available cars. */
    Dealership  UMETA(DisplayName = "Dealership"),

    /** Garage — view current car, manage skills, enter race. */
    Garage      UMETA(DisplayName = "Garage"),

    /**
     * Skill perks management — unlock and inspect driver perks.
     * Not yet implemented; navigation button is disabled.
     */
    Perks       UMETA(DisplayName = "Perks"),

    /**
     * Vehicle tuning — adjust part levels and gear ratios.
     * Not yet implemented; navigation button is disabled.
     */
    Tuning      UMETA(DisplayName = "Tuning"),

    /**
     * Vehicle collection — switch between owned cars.
     * Not yet implemented; navigation button is disabled.
     */
    Collection  UMETA(DisplayName = "Collection"),
};
