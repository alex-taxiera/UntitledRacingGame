// Copyright Epic Games, Inc. All Rights Reserved.

#include "NPCRacerData.h"
#include "OwnedVehicle.h"

// ---------------------------------------------------------------------------
// Runtime helpers
// ---------------------------------------------------------------------------

UCharacterPerkState* UNPCRacerData::CreatePerkState(UObject* Outer) const
{
    UCharacterPerkState* State = NewObject<UCharacterPerkState>(Outer);

    for (const FName& ID : PerkIDs)
    {
        State->AddPerk(ID);
    }

    return State;
}

FDriverStatBlock UNPCRacerData::ComputeStats(const TArray<UPerkData*>& AllPerks) const
{
    UCharacterPerkState* TempState = CreatePerkState(GetTransientPackage());
    return TempState->ComputeStats(AllPerks, BaseStats);
}

int32 UNPCRacerData::ComputeLevel(const TArray<UPerkData*>& AllPerks) const
{
    UCharacterPerkState* TempState = CreatePerkState(GetTransientPackage());
    return TempState->ComputeLevel(AllPerks);
}

UOwnedVehicle* UNPCRacerData::BuildOwnedVehicle(UObject* Outer) const
{
    UVehicleDefinition* Def = VehicleConfig.VehicleDefinition.Get();
    if (!Def)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UNPCRacerData::BuildOwnedVehicle — VehicleDefinition is null or not loaded for racer '%s'"),
            *RacerName.ToString());
        return nullptr;
    }

    // Create a transient UOwnedVehicle from the definition using the standard factory,
    // which seeds defaults for parts, gear ratios, and tuning.
    UOwnedVehicle* Vehicle = UOwnedVehicle::CreateFromDefinition(Outer, Def);

    // Apply designer-set part levels directly — no perk or currency gates for NPCs.
    for (const auto& Pair : VehicleConfig.InstalledPartLevels)
    {
        Vehicle->SetPartLevel(Pair.Key, Pair.Value);
    }

    // Apply gear ratio overrides if provided; otherwise the factory defaults remain.
    if (VehicleConfig.GearRatioOverrides.Num() > 0)
    {
        for (int32 i = 0; i < VehicleConfig.GearRatioOverrides.Num(); ++i)
        {
            Vehicle->SetGearRatio(i, VehicleConfig.GearRatioOverrides[i]);
        }
    }

    // Copy all tuning state directly — bypass unlock checks that only apply to players.
    Vehicle->AlignmentTuning  = VehicleConfig.AlignmentTuning;
    Vehicle->BrakeTuning      = VehicleConfig.BrakeTuning;
    Vehicle->RearLSDTuning    = VehicleConfig.RearLSDTuning;
    Vehicle->FrontLSDTuning   = VehicleConfig.FrontLSDTuning;
    Vehicle->SuspensionTuning = VehicleConfig.SuspensionTuning;
    Vehicle->StabilizerTuning = VehicleConfig.StabilizerTuning;
    Vehicle->TorqueBalance    = VehicleConfig.TorqueBalance;

    return Vehicle;
}
