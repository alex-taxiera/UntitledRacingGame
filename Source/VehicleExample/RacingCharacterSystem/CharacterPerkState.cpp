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
// Skill Slots & Equipping
// ---------------------------------------------------------------------------

int32 UCharacterPerkState::GetSkillSlotCount(const TArray<UPerkData*>& AllPerks,
                                              int32 BaseSlots) const
{
    int32 Total = BaseSlots;

    // SkillSlotIncrease effects from ALL unlocked perks count, not just equipped ones.
    // This prevents a chicken-and-egg problem where you need a slot to equip
    // the perk that gives you the extra slot.
    for (const FName& PerkID : UnlockedPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        for (const FPerkEffectData& Effect : Perk->GetAllEffects())
        {
            if (Effect.bIsPassive && Effect.EffectType == ESkillEffectType::SkillSlotIncrease)
            {
                Total += FMath::FloorToInt(Effect.Magnitude);
            }
        }
    }

    return FMath::Max(0, Total);
}

bool UCharacterPerkState::IsSkillEquipped(FName PerkID) const
{
    return EquippedSkillPerkIDs.Contains(PerkID);
}

bool UCharacterPerkState::CanEquipSkill(FName PerkID, const TArray<UPerkData*>& AllPerks,
                                         int32 BaseSlots) const
{
    if (!HasPerk(PerkID)) { return false; }
    if (IsSkillEquipped(PerkID)) { return false; }

    UPerkData* Perk = FindPerk(AllPerks, PerkID);
    if (!Perk) { return false; }

    // Only Driver-tree skill perks (perks with effects or a custom class) can be equipped.
    // Stat perks, vehicle perks, and tuning perks are passive by nature and never equipped.
    const bool bIsSkillPerk = (Perk->Tree == EPerkTree::Driver) &&
        (Perk->SkillPayload.Effects.Num() > 0 || !Perk->SkillPayload.CustomEffectClass.IsNull());
    if (!bIsSkillPerk) { return false; }

    return EquippedSkillPerkIDs.Num() < GetSkillSlotCount(AllPerks, BaseSlots);
}

bool UCharacterPerkState::EquipSkill(FName PerkID, const TArray<UPerkData*>& AllPerks,
                                      int32 BaseSlots)
{
    if (!CanEquipSkill(PerkID, AllPerks, BaseSlots)) { return false; }
    EquippedSkillPerkIDs.Add(PerkID);
    return true;
}

bool UCharacterPerkState::UnequipSkill(FName PerkID)
{
    return EquippedSkillPerkIDs.Remove(PerkID) > 0;
}

// ---------------------------------------------------------------------------
// Effect queries  (draw only from EquippedSkillPerkIDs)
// ---------------------------------------------------------------------------

TArray<FPerkEffectData> UCharacterPerkState::GetAllPassiveEffects(
    const TArray<UPerkData*>& AllPerks) const
{
    TArray<FPerkEffectData> Out;

    for (const FName& PerkID : EquippedSkillPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        for (const FPerkEffectData& Effect : Perk->GetAllEffects())
        {
            // SkillSlotIncrease is accounted for separately in GetSkillSlotCount.
            if (Effect.bIsPassive && Effect.EffectType != ESkillEffectType::SkillSlotIncrease)
            {
                Out.Add(Effect);
            }
        }
    }

    return Out;
}

TArray<FPerkEffectData> UCharacterPerkState::GetAllActiveEffects(
    const TArray<UPerkData*>& AllPerks) const
{
    TArray<FPerkEffectData> Out;

    for (const FName& PerkID : EquippedSkillPerkIDs)
    {
        UPerkData* Perk = FindPerk(AllPerks, PerkID);
        if (!Perk) { continue; }

        for (const FPerkEffectData& Effect : Perk->GetAllEffects())
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

    for (const FName& PerkID : EquippedSkillPerkIDs)
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
