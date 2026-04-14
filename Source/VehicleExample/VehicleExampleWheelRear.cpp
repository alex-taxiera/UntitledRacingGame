// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleExampleWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UVehicleExampleWheelRear::UVehicleExampleWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}