// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VehicleDefinition.h"
#include "SportsCarVehicleDefinition.generated.h"

/**
 * USportsCarVehicleDefinition
 *
 * A UVehicleDefinition subclass that ships with all fields pre-filled with
 * sensible defaults for the built-in sports car asset.
 *
 * How to create the Data Asset in the editor:
 *   1. Content Browser ? right-click ? Miscellaneous ? Data Asset.
 *   2. Pick "SportsCarVehicleDefinition" as the class.
 *   3. All fields will already be populated — assign PreviewMesh and PawnClass
 *      by pointing them at the existing sports car skeletal mesh and Blueprint.
 *   4. Add this asset to URacingGameInstance ? VehicleInventory ? AllVehicles.
 *
 * Stats are loosely modelled on a ~300 HP rear-wheel-drive sports coupe.
 * Designers can override any individual field in the asset editor.
 *
 * Part ladders:  4 levels per slot (Stock ? Sport ? Super ? Racing)
 * Gearbox:       6 forward gears, realistic close-ratio spread
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API USportsCarVehicleDefinition : public UVehicleDefinition
{
    GENERATED_BODY()

public:

    USportsCarVehicleDefinition();

    virtual void PostInitProperties() override;
};
