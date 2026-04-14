// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerPerkManager.h"
#include "VehicleDefinition.h"

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

void UPlayerPerkManager::Initialise()
{
    if (!PerkState)
    {
        PerkState = NewObject<UCharacterPerkState>(this, TEXT("PlayerPerkState"));
    }
}

// ---------------------------------------------------------------------------
// Skill Points
// ---------------------------------------------------------------------------

void UPlayerPerkManager::AddSkillPoints(int32 Amount)
{
    SkillPointBank = FMath::Max(0, SkillPointBank + Amount);
    OnSkillPointsChanged.Broadcast(SkillPointBank);
}

// ---------------------------------------------------------------------------
// Perk Purchase
// ---------------------------------------------------------------------------

bool UPlayerPerkManager::CanUnlockPerk(UPerkData* Perk) const
{
    if (!Perk || !PerkState) { return false; }
    if (PerkState->HasPerk(Perk->PerkID)) { return false; }
    if (SkillPointBank < Perk->PointCost) { return false; }
    if (!PerkState->ArePrerequisitesMet(Perk, AchievedStoryFlags)) { return false; }
    return true;
}

bool UPlayerPerkManager::UnlockPerk(UPerkData* Perk)
{
    if (!CanUnlockPerk(Perk)) { return false; }

    SkillPointBank -= Perk->PointCost;
    OnSkillPointsChanged.Broadcast(SkillPointBank);

    PerkState->AddPerk(Perk->PerkID);
    OnPerkUnlocked.Broadcast(Perk);

    return true;
}

// ---------------------------------------------------------------------------
// Story Flags
// ---------------------------------------------------------------------------

void UPlayerPerkManager::AddStoryFlag(FName Flag)
{
    if (!Flag.IsNone())
    {
        AchievedStoryFlags.Add(Flag);
    }
}

bool UPlayerPerkManager::HasStoryFlag(FName Flag) const
{
    return AchievedStoryFlags.Contains(Flag);
}

// ---------------------------------------------------------------------------
// Shop Filter Queries
// ---------------------------------------------------------------------------

TArray<UVehicleDefinition*> UPlayerPerkManager::GetUnlockedVehicles() const
{
    TArray<UVehicleDefinition*> Out;
    if (!PerkState) { return Out; }

    for (UPerkData* Perk : AllPerks)
    {
        if (!Perk || Perk->Tree != EPerkTree::Vehicle) { continue; }
        if (!PerkState->HasPerk(Perk->PerkID)) { continue; }

        UVehicleDefinition* Def = Perk->VehiclePayload.VehicleDefinition.Get();
        if (Def)
        {
            Out.Add(Def);
        }
    }

    return Out;
}

bool UPlayerPerkManager::IsPartLevelPurchasable(EPartSlot Slot, int32 Level) const
{
    if (!PerkState) { return false; }

    // Level 0 (stock) is always purchaseable — no perk required
    if (Level <= 0) { return true; }

    for (UPerkData* Perk : AllPerks)
    {
        if (!Perk || Perk->Tree != EPerkTree::Tuning) { continue; }
        if (Perk->TuningPayload.PartSlot != Slot) { continue; }
        if (Perk->TuningPayload.PartLevel != Level) { continue; }
        if (PerkState->HasPerk(Perk->PerkID)) { return true; }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Convenience wrappers
// ---------------------------------------------------------------------------

int32 UPlayerPerkManager::GetPlayerLevel() const
{
    if (!PerkState) { return 0; }
    return PerkState->ComputeLevel(GetRawPerkArray());
}

FDriverStatBlock UPlayerPerkManager::GetPlayerStats(const FDriverStatBlock& BaseStats) const
{
    if (!PerkState) { return BaseStats; }
    return PerkState->ComputeStats(GetRawPerkArray(), BaseStats);
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

TArray<UPerkData*> UPlayerPerkManager::GetRawPerkArray() const
{
    TArray<UPerkData*> Out;
    Out.Reserve(AllPerks.Num());
    for (const TObjectPtr<UPerkData>& P : AllPerks)
    {
        if (P) { Out.Add(P.Get()); }
    }
    return Out;
}
