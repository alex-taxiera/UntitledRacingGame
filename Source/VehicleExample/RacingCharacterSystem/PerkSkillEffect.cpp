// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerkSkillEffect.h"

void UPerkSkillEffect::ApplyEffect(AActor* InOwner, const FPerkEffectData& InData)
{
    OwningActor = InOwner;
    EffectData  = InData;
    OnEffectApplied();
}

void UPerkSkillEffect::RemoveEffect()
{
    OnEffectRemoved();
    OwningActor = nullptr;
}
