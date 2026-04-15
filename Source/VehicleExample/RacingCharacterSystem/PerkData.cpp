// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerkData.h"

TArray<FPerkEffectData> UPerkData::GetAllEffects() const
{
    TArray<FPerkEffectData> Out;

    if (Tree == EPerkTree::Driver)
    {
        Out.Append(SkillPayload.Effects);
    }
    else if (Tree == EPerkTree::Perks)
    {
        Out.Append(GlobalPerkPayload.Effects);
    }

    return Out;
}

TSoftClassPtr<UPerkSkillEffect> UPerkData::GetCustomEffectClass() const
{
    if (Tree == EPerkTree::Driver && !SkillPayload.CustomEffectClass.IsNull())
    {
        return SkillPayload.CustomEffectClass;
    }
    if (Tree == EPerkTree::Perks && !GlobalPerkPayload.CustomEffectClass.IsNull())
    {
        return GlobalPerkPayload.CustomEffectClass;
    }
    return TSoftClassPtr<UPerkSkillEffect>();
}
