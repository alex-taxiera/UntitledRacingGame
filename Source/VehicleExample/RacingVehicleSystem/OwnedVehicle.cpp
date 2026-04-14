// Copyright Epic Games, Inc. All Rights Reserved.

#include "OwnedVehicle.h"
#include "VehiclePartData.h"

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

UOwnedVehicle* UOwnedVehicle::CreateFromDefinition(UObject* Outer, UVehicleDefinition* InDefinition)
{
    check(InDefinition);

    UOwnedVehicle* Instance = NewObject<UOwnedVehicle>(Outer);
    Instance->InstanceID   = FGuid::NewGuid();
    Instance->Definition   = InDefinition;

    // Stock level (0) for every supported slot
    for (const auto& Pair : InDefinition->AvailableParts)
    {
        Instance->InstalledPartLevels.Add(Pair.Key, 0);
    }

    // Default gear ratios from the definition
    Instance->ResetGearRatiosToDefault();

    return Instance;
}

// ---------------------------------------------------------------------------
// Part Queries & Mutations
// ---------------------------------------------------------------------------

int32 UOwnedVehicle::GetInstalledLevel(EPartSlot Slot) const
{
    const int32* Found = InstalledPartLevels.Find(Slot);
    return Found ? *Found : 0;
}

bool UOwnedVehicle::SetPartLevel(EPartSlot Slot, int32 Level)
{
    if (!Definition) { return false; }

    UVehiclePartData* PartData = Definition->GetPartData(Slot);
    if (!PartData || !PartData->IsValidLevel(Level)) { return false; }

    InstalledPartLevels.FindOrAdd(Slot) = Level;

    // Transmission changes may alter the number of available gears
    if (Slot == EPartSlot::Transmission)
    {
        RefreshGearCount();
    }

    return true;
}

// ---------------------------------------------------------------------------
// Gear Ratio Tuning
// ---------------------------------------------------------------------------

bool UOwnedVehicle::SetGearRatio(int32 GearIndex, float Ratio)
{
    if (!TunedGearRatios.IsValidIndex(GearIndex) || !Definition) { return false; }

    const FGearRatioSpec Spec = Definition->GetGearRatioSpec(GearIndex);
    TunedGearRatios[GearIndex] = FMath::Clamp(Ratio, Spec.MinRatio, Spec.MaxRatio);
    return true;
}

void UOwnedVehicle::ResetGearRatiosToDefault()
{
    if (!Definition) { return; }

    // Determine gear count from installed Transmission level (if any)
    const int32 TransLevel = GetInstalledLevel(EPartSlot::Transmission);
    UVehiclePartData* TransPart = Definition->GetPartData(EPartSlot::Transmission);

    int32 GearCount = Definition->DefaultGearRatios.Num();
    if (TransPart && TransPart->IsValidLevel(TransLevel))
    {
        const FPartLevelData* LevelData = TransPart->GetLevelData(TransLevel);
        if (LevelData && LevelData->GearCount > 0)
        {
            GearCount = LevelData->GearCount;
        }
    }

    TunedGearRatios.SetNum(GearCount);
    for (int32 i = 0; i < GearCount; ++i)
    {
        TunedGearRatios[i] = Definition->GetGearRatioSpec(i).DefaultRatio;
    }
}

// ---------------------------------------------------------------------------
// Effective Stats
// ---------------------------------------------------------------------------

FEffectiveVehicleStats UOwnedVehicle::ComputeEffectiveStats() const
{
    FEffectiveVehicleStats Stats;

    if (!Definition) { return Stats; }

    // Seed from base stats and engine definition
    Stats.PowerHP          = Definition->EngineDefinition.BasePowerHP;
    Stats.TorqueNm         = Definition->EngineDefinition.BaseTorqueNm;
    Stats.MaxRPM           = Definition->EngineDefinition.MaxRPM;
    Stats.MassKg           = Definition->BaseStats.MassKg;
    Stats.DragCoefficient  = Definition->BaseStats.DragCoefficient;
    Stats.GripMultiplier   = Definition->BaseStats.BaseGripMultiplier;
    Stats.BrakingEfficiency = 1.0f;
    Stats.NitroCapacity    = 0.0f;
    Stats.NitroForce       = 0.0f;

    // Accumulate modifiers from all installed parts
    for (const auto& Pair : InstalledPartLevels)
    {
        const EPartSlot Slot  = Pair.Key;
        const int32     Level = Pair.Value;

        UVehiclePartData* PartData = Definition->GetPartData(Slot);
        if (!PartData) { continue; }

        const FPartLevelData* LevelData = PartData->GetLevelData(Level);
        if (!LevelData) { continue; }

        const FPartStatModifiers& Mod = LevelData->StatModifiers;

        Stats.PowerHP          += Mod.PowerHP;
        Stats.TorqueNm         += Mod.TorqueNm;
        Stats.MaxRPM           *= (1.0f + Mod.MaxRPMMultiplier);
        Stats.BrakingEfficiency += Mod.BrakingEfficiencyBonus;
        Stats.MassKg           -= Mod.WeightReductionKg;
        Stats.GripMultiplier   += Mod.GripBonus;
        Stats.DragCoefficient  += Mod.DragDelta;
        Stats.NitroCapacity    += Mod.NitroCapacity;
        Stats.NitroForce       += Mod.NitroForce;
    }

    // Copy tuned gear ratios
    Stats.GearRatios = TunedGearRatios;

    return Stats;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

FString UOwnedVehicle::GetDisplayName() const
{
    if (!Nickname.IsEmpty()) { return Nickname; }
    return Definition ? Definition->DisplayName.ToString() : TEXT("Unknown Vehicle");
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void UOwnedVehicle::RefreshGearCount()
{
    if (!Definition) { return; }

    const int32 TransLevel = GetInstalledLevel(EPartSlot::Transmission);
    UVehiclePartData* TransPart = Definition->GetPartData(EPartSlot::Transmission);

    int32 NewGearCount = Definition->DefaultGearRatios.Num();
    if (TransPart && TransPart->IsValidLevel(TransLevel))
    {
        const FPartLevelData* LevelData = TransPart->GetLevelData(TransLevel);
        if (LevelData && LevelData->GearCount > 0)
        {
            NewGearCount = LevelData->GearCount;
        }
    }

    const int32 OldCount = TunedGearRatios.Num();
    TunedGearRatios.SetNum(NewGearCount);

    // Fill any newly added gears with their definition defaults
    for (int32 i = OldCount; i < NewGearCount; ++i)
    {
        TunedGearRatios[i] = Definition->GetGearRatioSpec(i).DefaultRatio;
    }
}
