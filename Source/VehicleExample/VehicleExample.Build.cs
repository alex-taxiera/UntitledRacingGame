// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VehicleExample : ModuleRules
{
	public VehicleExample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"AIModule"
		});

		PublicIncludePaths.AddRange(new string[] {
			"VehicleExample",
			"VehicleExample/SportsCar",
			"VehicleExample/OffroadCar",
			"VehicleExample/Variant_Offroad",
			"VehicleExample/Variant_TimeTrial",
			"VehicleExample/Variant_TimeTrial/UI",
			"VehicleExample/RacingVehicleSystem",
			"VehicleExample/RacingCharacterSystem",
			"VehicleExample/RacingAISystem",
			"VehicleExample/UI"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UMG",
			"RenderCore",
			"Renderer",
			"OnlineSubsystem",
			"OnlineSubsystemNull",
			"OnlineSubsystemUtils"
		});
	}
}
