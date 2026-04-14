// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PerkTypes.h"
#include "PerkSkillEffect.generated.h"

/**
 * UPerkSkillEffect
 *
 * Base class for custom Blueprint-implemented perk effects.
 * Used when a perk's behaviour cannot be expressed with a known ESkillEffectType.
 *
 * Workflow:
 *   1. In the Content Browser, create a Blueprint subclass of UPerkSkillEffect.
 *   2. Override OnEffectApplied and OnEffectRemoved to implement the custom logic.
 *   3. Reference the Blueprint class in the perk's SkillPayload.CustomEffectClass
 *      or GlobalPerkPayload.CustomEffectClass field.
 *
 * The battle system will:
 *   - Call ApplyEffect (which triggers OnEffectApplied) at race start or on unlock.
 *   - Call RemoveEffect (which triggers OnEffectRemoved) when the effect expires
 *     or the race ends.
 *
 * For passive effects, ApplyEffect is called when the perk is equipped and
 * RemoveEffect is called only if the perk were somehow removed (future feature).
 */
UCLASS(Blueprintable, BlueprintType)
class VEHICLEEXAMPLE_API UPerkSkillEffect : public UObject
{
    GENERATED_BODY()

public:

    /**
     * The underlying effect data from the perk, set by the system before Apply is called.
     * Blueprint subclasses can read this to know the magnitude / duration.
     */
    UPROPERTY(BlueprintReadOnly, Category = "PerkEffect")
    FPerkSkillEffect EffectData;

    /**
     * The actor (pawn) this effect is acting on.
     * Set by the battle system before Apply is called.
     */
    UPROPERTY(BlueprintReadOnly, Category = "PerkEffect")
    TObjectPtr<AActor> OwningActor;

    // -----------------------------------------------------------------------
    // C++ entry points — called by the battle system
    // -----------------------------------------------------------------------

    /** Activates this effect. Calls the Blueprint event OnEffectApplied. */
    void ApplyEffect(AActor* InOwner, const FPerkSkillEffect& InData);

    /** Deactivates this effect. Calls the Blueprint event OnEffectRemoved. */
    void RemoveEffect();

    // -----------------------------------------------------------------------
    // Blueprint implementable hooks
    // -----------------------------------------------------------------------

    /**
     * Called when this effect becomes active.
     * Override in Blueprint to implement custom behaviour.
     * EffectData and OwningActor are set before this fires.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PerkEffect")
    void OnEffectApplied();

    /**
     * Called when this effect is deactivated (duration expired or race ended).
     * Override in Blueprint to clean up any state applied in OnEffectApplied.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PerkEffect")
    void OnEffectRemoved();

    /**
     * Optional tick for active effects that need per-frame updates
     * (e.g. a damage-over-time or periodic heal).
     * Only called if bWantsTick is true. Off by default for performance.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "PerkEffect")
    void OnEffectTick(float DeltaSeconds);

    /**
     * Set to true in Blueprint if this effect needs OnEffectTick to be called.
     * Keep false (default) for all effects that don't require a tick.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PerkEffect")
    bool bWantsTick = false;
};
