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

    // Default tuning settings from the definition
    Instance->ResetTuningToDefaults();

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

// ---------------------------------------------------------------------------
// Tuning — Reset
// ---------------------------------------------------------------------------

void UOwnedVehicle::ResetTuningToDefaults()
{
    if (!Definition) { return; }

    const FAlignmentTuningDef& A = Definition->AlignmentTuning;
    AlignmentTuning.CamberFront    = A.CamberFront.Default;
    AlignmentTuning.CamberRear     = A.CamberRear.Default;
    AlignmentTuning.ToeFront       = A.ToeFront.Default;
    AlignmentTuning.ToeRear        = A.ToeRear.Default;
    AlignmentTuning.RideHeightFront = A.RideHeightFront.Default;
    AlignmentTuning.RideHeightRear  = A.RideHeightRear.Default;
    AlignmentTuning.OffsetFront    = A.OffsetFront.Default;
    AlignmentTuning.OffsetRear     = A.OffsetRear.Default;
    AlignmentTuning.TireWidthFront = A.TireWidthFront.Default;
    AlignmentTuning.TireWidthRear  = A.TireWidthRear.Default;

    const FBrakeTuningDef& B = Definition->BrakeTuning;
    BrakeTuning.bABSEnabled  = false;
    BrakeTuning.BrakeBalance = B.BrakeBalance.Default;

    const FLSDTuningDef& RL = Definition->RearLSDTuning;
    RearLSDTuning.LSDType       = RL.DefaultLSDType;
    RearLSDTuning.InitialTorque = RL.InitialTorque.Default;
    RearLSDTuning.LSDRatio      = RL.LSDRatio.Default;

    const FLSDTuningDef& FL = Definition->FrontLSDTuning;
    FrontLSDTuning.LSDType       = FL.DefaultLSDType;
    FrontLSDTuning.InitialTorque = FL.InitialTorque.Default;
    FrontLSDTuning.LSDRatio      = FL.LSDRatio.Default;

    const FSuspensionTuningDef& S = Definition->SuspensionTuning;
    SuspensionTuning.SpringRateFront = S.SpringRateFront.Default;
    SuspensionTuning.SpringRateRear  = S.SpringRateRear.Default;
    SuspensionTuning.DamperFront     = S.DamperFront.Default;
    SuspensionTuning.DamperRear      = S.DamperRear.Default;
    SuspensionTuning.DamperBalance   = S.DamperBalance.Default;

    StabilizerTuning.StabilizerFront = Definition->StabilizerTuning.StabilizerFront.Default;
    StabilizerTuning.StabilizerRear  = Definition->StabilizerTuning.StabilizerRear.Default;

    TorqueBalance.FrontBias = Definition->TorqueBalance.DefaultFrontBias;
}

// ---------------------------------------------------------------------------
// Tuning — Unlock checks
// ---------------------------------------------------------------------------

bool UOwnedVehicle::IsAlignmentTuningUnlocked() const
{
    if (!Definition) { return false; }
    return GetInstalledLevel(EPartSlot::Suspension) >= Definition->AlignmentTuning.MinSuspensionLevelForAlignment;
}

bool UOwnedVehicle::IsTireWidthTuningUnlocked() const
{
    if (!Definition) { return false; }
    return GetInstalledLevel(EPartSlot::Tire) >= Definition->AlignmentTuning.MinTireLevelForTireWidth;
}

bool UOwnedVehicle::IsBrakeTuningUnlocked() const
{
    if (!Definition) { return false; }
    return GetInstalledLevel(EPartSlot::Brake) >= Definition->BrakeTuning.MinBrakeLevelForTuning;
}

bool UOwnedVehicle::IsRearLSDTuningUnlocked() const
{
    if (!Definition) { return false; }
    return GetInstalledLevel(EPartSlot::LSD) >= Definition->RearLSDTuning.MinLSDLevelForTuning;
}

bool UOwnedVehicle::IsFrontLSDTuningUnlocked() const
{
    if (!Definition || !Definition->IsAWD()) { return false; }
    return GetInstalledLevel(EPartSlot::LSD) >= Definition->FrontLSDTuning.MinLSDLevelForTuning;
}

bool UOwnedVehicle::IsSuspensionTuningUnlocked() const
{
    if (!Definition) { return false; }
    return GetInstalledLevel(EPartSlot::Suspension) >= Definition->SuspensionTuning.MinSuspensionLevelForTuning;
}

bool UOwnedVehicle::IsStabilizerTuningUnlocked() const
{
    // Shares the same gate as suspension tuning
    return IsSuspensionTuningUnlocked();
}

bool UOwnedVehicle::IsTorqueBalanceTuningUnlocked() const
{
    return Definition && Definition->IsAWD();
}

// ---------------------------------------------------------------------------
// Tuning — Setters
// ---------------------------------------------------------------------------

bool UOwnedVehicle::SetCamberFront(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.CamberFront;
    AlignmentTuning.CamberFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetCamberRear(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.CamberRear;
    AlignmentTuning.CamberRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetToeFront(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.ToeFront;
    AlignmentTuning.ToeFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetToeRear(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.ToeRear;
    AlignmentTuning.ToeRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetRideHeightFront(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.RideHeightFront;
    AlignmentTuning.RideHeightFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetRideHeightRear(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.RideHeightRear;
    AlignmentTuning.RideHeightRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetOffsetFront(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.OffsetFront;
    AlignmentTuning.OffsetFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetOffsetRear(float Value)
{
    if (!IsAlignmentTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.OffsetRear;
    AlignmentTuning.OffsetRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetTireWidthFront(float Value)
{
    if (!IsTireWidthTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.TireWidthFront;
    AlignmentTuning.TireWidthFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetTireWidthRear(float Value)
{
    if (!IsTireWidthTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->AlignmentTuning.TireWidthRear;
    AlignmentTuning.TireWidthRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetABSEnabled(bool bEnabled)
{
    if (!IsBrakeTuningUnlocked()) { return false; }
    if (!Definition->BrakeTuning.bABSAvailable) { return false; }
    BrakeTuning.bABSEnabled = bEnabled;
    return true;
}

bool UOwnedVehicle::SetBrakeBalance(float Value)
{
    if (!IsBrakeTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->BrakeTuning.BrakeBalance;
    BrakeTuning.BrakeBalance = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetRearLSDType(ELSDType Type)
{
    if (!IsRearLSDTuningUnlocked()) { return false; }
    RearLSDTuning.LSDType = Type;
    return true;
}

bool UOwnedVehicle::SetRearLSDInitialTorque(float Value)
{
    if (!IsRearLSDTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->RearLSDTuning.InitialTorque;
    RearLSDTuning.InitialTorque = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetRearLSDRatio(float Value)
{
    if (!IsRearLSDTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->RearLSDTuning.LSDRatio;
    RearLSDTuning.LSDRatio = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetFrontLSDType(ELSDType Type)
{
    if (!IsFrontLSDTuningUnlocked()) { return false; }
    FrontLSDTuning.LSDType = Type;
    return true;
}

bool UOwnedVehicle::SetFrontLSDInitialTorque(float Value)
{
    if (!IsFrontLSDTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->FrontLSDTuning.InitialTorque;
    FrontLSDTuning.InitialTorque = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetFrontLSDRatio(float Value)
{
    if (!IsFrontLSDTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->FrontLSDTuning.LSDRatio;
    FrontLSDTuning.LSDRatio = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetSpringRateFront(float Value)
{
    if (!IsSuspensionTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->SuspensionTuning.SpringRateFront;
    SuspensionTuning.SpringRateFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetSpringRateRear(float Value)
{
    if (!IsSuspensionTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->SuspensionTuning.SpringRateRear;
    SuspensionTuning.SpringRateRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetDamperFront(float Value)
{
    if (!IsSuspensionTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->SuspensionTuning.DamperFront;
    SuspensionTuning.DamperFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetDamperRear(float Value)
{
    if (!IsSuspensionTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->SuspensionTuning.DamperRear;
    SuspensionTuning.DamperRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetDamperBalance(float Value)
{
    if (!IsSuspensionTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->SuspensionTuning.DamperBalance;
    SuspensionTuning.DamperBalance = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetStabilizerFront(float Value)
{
    if (!IsStabilizerTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->StabilizerTuning.StabilizerFront;
    StabilizerTuning.StabilizerFront = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetStabilizerRear(float Value)
{
    if (!IsStabilizerTuningUnlocked()) { return false; }
    const FTuningSpec& Spec = Definition->StabilizerTuning.StabilizerRear;
    StabilizerTuning.StabilizerRear = FMath::Clamp(Value, Spec.Min, Spec.Max);
    return true;
}

bool UOwnedVehicle::SetTorqueBalanceFrontBias(int32 FrontBias)
{
    if (!IsTorqueBalanceTuningUnlocked()) { return false; }
    const FTorqueBalanceDef& Def = Definition->TorqueBalance;
    TorqueBalance.FrontBias = FMath::Clamp(FrontBias, Def.MinFrontBias, Def.MaxFrontBias);
    return true;
}
}
