// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleExampleGameMode.h"
#include "VehicleExamplePlayerController.h"

AVehicleExampleGameMode::AVehicleExampleGameMode()
{
	PlayerControllerClass = AVehicleExamplePlayerController::StaticClass();
}
