// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NPCRacerData.h"
#include "SportsCarNPCRacerData.generated.h"

/**
 * USportsCarNPCRacerData
 *
 * A UNPCRacerData subclass pre-configured as a balanced first rival.
 * Drives the same Type-SR Sport Coupe the player starts with, at stock
 * parts, with moderate combat stats and a straightforward AI profile.
 *
 * How to create the Data Asset in the editor:
 *   1. Content Browser ? right-click ? Miscellaneous ? Data Asset.
 *   2. Pick "SportsCarNPCRacerData" as the class.
 *   3. All fields will be pre-filled. Set VehicleConfig.VehicleDefinition
 *      to your USportsCarVehicleDefinition data asset.
 *   4. Optionally assign a Portrait texture.
 *
 * Designers can freely override any field directly in the asset editor.
 */
UCLASS(BlueprintType)
class VEHICLEEXAMPLE_API USportsCarNPCRacerData : public UNPCRacerData
{
    GENERATED_BODY()

public:

    USportsCarNPCRacerData();

    virtual void PostInitProperties() override;
};
