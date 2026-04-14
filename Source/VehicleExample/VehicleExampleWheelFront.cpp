// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleExampleWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UVehicleExampleWheelFront::UVehicleExampleWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}