// Copyright Epic Games, Inc. All Rights Reserved.

#include "CharacterPerkState.h"

// ---------------------------------------------------------------------------
// Unlock state
// ---------------------------------------------------------------------------

bool UCharacterPerkState::HasPerk(FName PerkID) const
{
    return UnlockedPerkIDs.Contains(PerkID);
}

void UCharacterPerkState::AddPerk(FName PerkID)
{
    if (!PerkID.IsNone() && !UnlockedPerkIDs.Contains(PerkID))
    {
        UnlockedPerkIDs.Add(PerkID);
    }
}

// ---------------------------------------------------------------------------
// Stat computation
// ---------------------------------------------------------------------------

FDriverStatBlock UCharacterPerkState::ComputeStats(const TArray<UPerkData*>& AllPerks,
                                                    const FDriverStatBlock& BaseStats) const
{
    FDriverStatBlock Result = BaseStats;

    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk || !Perk->IsStatPerk()) { continue; }

        switch (Perk->StatPayload.Stat)
        {
            case EDriverStat::Attack:    Result.Attack    += Perk->StatPayload.Magnitude; break;
            case EDriverStat::Defense:   Result.Defense   += Perk->StatPayload.Magnitude; break;
            case EDriverStat::Health:    Result.Health    += Perk->StatPayload.Magnitude; break;
            case EDriverStat::Toughness: Result.Toughness += Perk->StatPayload.Magnitude; break;
        }
    }

    return Result;
}

int32 UCharacterPerkState::ComputeLevel(const TArray<UPerkData*>& AllPerks) const
{
    int32 Level = 0;

    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (Perk && Perk->IsStatPerk())
        {
            ++Level;
        }
    }

    return Level;
}

// ---------------------------------------------------------------------------
// Effect queries
// ---------------------------------------------------------------------------

TArray<FPerkSkillEffect> UCharacterPerkState::GetAllPassiveEffects(
    const TArray<UPerkData*>& AllPerks) const
{
    TArray<FPerkSkillEffect> Out;

    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        for (const FPerkSkillEffect& Effect : Perk->GetAllEffects())
        {
            if (Effect.bIsPassive)
            {
                Out.Add(Effect);
            }
        }
    }

    return Out;
}

TArray<FPerkSkillEffect> UCharacterPerkState::GetAllActiveEffects(
    const TArray<UPerkData*>& AllPerks) const
{
    TArray<FPerkSkillEffect> Out;

    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        for (const FPerkSkillEffect& Effect : Perk->GetAllEffects())
        {
            if (!Effect.bIsPassive)
            {
                Out.Add(Effect);
            }
        }
    }

    return Out;
}

TArray<TSoftClassPtr<UPerkSkillEffect>> UCharacterPerkState::GetAllCustomEffectClasses(
    const TArray<UPerkData*>& AllPerks) const
{
    TArray<TSoftClassPtr<UPerkSkillEffect>> Out;

    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        const TSoftClassPtr<UPerkSkillEffect> CustomClass = Perk->GetCustomEffectClass();
        if (!CustomClass.IsNull())
        {
            Out.Add(CustomClass);
        }
    }

    return Out;
}

// ---------------------------------------------------------------------------
// Prerequisite check
// ---------------------------------------------------------------------------

bool UCharacterPerkState::ArePrerequisitesMet(const UPerkData* Perk,
                                               const TSet<FName>& AchievedStoryFlags) const
{
    if (!Perk) { return false; }

    for (const FName& RequiredID : Perk->PrerequisitePerkIDs)
    {
        if (!UnlockedPerkIDs.Contains(RequiredID)) { return false; }
    }

    for (const FStoryProgressionRequirement& Req : Perk->StoryRequirements)
    {
        if (!AchievedStoryFlags.Contains(Req.Flag)) { return false; }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

UPerkData* UCharacterPerkState::FindPerk(const TArray<UPerkData*>& AllPerks, FName PerkID)
{
    for (UPerkData* Perk : AllPerks)
    {
        if (Perk && Perk->PerkID == PerkID) { return Perk; }
    }
    return nullptr;
}
