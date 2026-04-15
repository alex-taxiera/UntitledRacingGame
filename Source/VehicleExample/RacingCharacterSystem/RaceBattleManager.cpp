// Copyright Epic Games, Inc. All Rights Reserved.

#include "RaceBattleManager.h"
#include "PlayerPerkManager.h"
#include "NPCRacerData.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void URaceBattleManager::StartBattle(UPlayerPerkManager* InPlayerManager,
                                      const FDriverStatBlock& InPlayerBaseStats,
                                      const FText& InPlayerName,
                                      UNPCRacerData* InOpponent,
                                      const TArray<UPerkData*>& InAllPerks,
                                      UObject* EffectOuter)
{
    if (!InPlayerManager || !InOpponent) { return; }

    RaceElapsedSeconds = 0.0f;

    // --- Player ---
    FDriverStatBlock PlayerResolved = InPlayerManager->GetPlayerStats(InPlayerBaseStats);
    UCharacterPerkState* PlayerPerkState = InPlayerManager->PerkState;
    InitialiseRacerState(PlayerBattleState, InPlayerName, PlayerResolved,
                         PlayerPerkState, InAllPerks, EffectOuter);

    // --- Opponent ---
    FDriverStatBlock OpponentResolved = InOpponent->ComputeStats(InAllPerks);
    UCharacterPerkState* OpponentPerkState = InOpponent->CreatePerkState(EffectOuter);
    InitialiseRacerState(OpponentBattleState, InOpponent->RacerName, OpponentResolved,
                         OpponentPerkState, InAllPerks, EffectOuter);

    bBattleActive = true;
    OnBattleStarted.Broadcast();
}

void URaceBattleManager::EndBattle()
{
    if (!bBattleActive) { return; }

    bBattleActive = false;

    // Clean up live custom effects for both racers
    for (UPerkSkillEffect* Effect : PlayerBattleState.LiveCustomEffects)
    {
        if (Effect) { Effect->RemoveEffect(); }
    }
    PlayerBattleState.LiveCustomEffects.Empty();

    for (UPerkSkillEffect* Effect : OpponentBattleState.LiveCustomEffects)
    {
        if (Effect) { Effect->RemoveEffect(); }
    }
    OpponentBattleState.LiveCustomEffects.Empty();

    OnBattleEnded.Broadcast();
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void URaceBattleManager::Tick(float DeltaSeconds)
{
    if (!bBattleActive) { return; }

    RaceElapsedSeconds += DeltaSeconds;

    TickTimedEffects(PlayerBattleState,   true,  DeltaSeconds);
    TickTimedEffects(OpponentBattleState, false, DeltaSeconds);

    // Tick any custom effects that want per-frame updates
    for (UPerkSkillEffect* Effect : PlayerBattleState.LiveCustomEffects)
    {
        if (Effect && Effect->bWantsTick) { Effect->OnEffectTick(DeltaSeconds); }
    }
    for (UPerkSkillEffect* Effect : OpponentBattleState.LiveCustomEffects)
    {
        if (Effect && Effect->bWantsTick) { Effect->OnEffectTick(DeltaSeconds); }
    }
}

// ---------------------------------------------------------------------------
// Health / Damage
// ---------------------------------------------------------------------------

void URaceBattleManager::ApplyDistanceDamage(bool bPlayerIsBehind, float DistanceFraction)
{
    if (!bBattleActive) { return; }

    FRacerBattleState& Defender = GetState(bPlayerIsBehind);
    FRacerBattleState& Attacker = GetState(!bPlayerIsBehind);

    const float BaseDamage = static_cast<float>(Attacker.ResolvedStats.Attack) * DistanceFraction;
    const float Reduction  = static_cast<float>(Defender.ResolvedStats.Defense)
                           + Defender.GetTotalDamageReduction();
    const float Final = FMath::Max(0.0f, BaseDamage - Reduction);

    ModifyHealth(Defender, bPlayerIsBehind, -Final);
}

void URaceBattleManager::ApplyWallDamage(bool bPlayerHit, float ImpactForce)
{
    if (!bBattleActive) { return; }

    FRacerBattleState& Target = GetState(bPlayerHit);

    const float Reduction = static_cast<float>(Target.ResolvedStats.Toughness)
                          + Target.GetTotalWallDamageReduction();
    const float Final = FMath::Max(0.0f, ImpactForce - Reduction);

    ModifyHealth(Target, bPlayerHit, -Final);
}

void URaceBattleManager::HealRacer(bool bPlayer, float Amount)
{
    if (!bBattleActive || Amount <= 0.0f) { return; }
    ModifyHealth(GetState(bPlayer), bPlayer, Amount);
}

void URaceBattleManager::OnNitroUsed(bool bPlayer)
{
    if (!bBattleActive) { return; }

    FRacerBattleState& State = GetState(bPlayer);
    const float HealAmount = State.GetTotalHealthOnNitro();

    if (HealAmount > 0.0f)
    {
        ModifyHealth(State, bPlayer, HealAmount);
    }
}

// ---------------------------------------------------------------------------
// Display Queries
// ---------------------------------------------------------------------------

FText URaceBattleManager::GetDisplayName(bool bPlayer) const
{
    return GetState(bPlayer).DisplayName;
}

float URaceBattleManager::GetCurrentHealth(bool bPlayer) const
{
    return GetState(bPlayer).CurrentHealth;
}

float URaceBattleManager::GetMaxHealth(bool bPlayer) const
{
    return GetState(bPlayer).MaxHealth;
}

float URaceBattleManager::GetHealthPercent(bool bPlayer) const
{
    const FRacerBattleState& State = GetState(bPlayer);
    if (State.MaxHealth <= 0.0f) { return 0.0f; }
    return State.CurrentHealth / State.MaxHealth;
}

TArray<FText> URaceBattleManager::GetEquippedSkillNames(bool bPlayer,
                                                          const TArray<UPerkData*>& AllPerks) const
{
    TArray<FText> Out;
    for (const FName& ID : GetState(bPlayer).EquippedSkillPerkIDs)
    {
        for (UPerkData* Perk : AllPerks)
        {
            if (Perk && Perk->PerkID == ID)
            {
                Out.Add(Perk->DisplayName);
                break;
            }
        }
    }
    return Out;
}

TArray<FText> URaceBattleManager::GetActiveTimedEffectNames(bool bPlayer,
                                                              const TArray<UPerkData*>& AllPerks) const
{
    TArray<FText> Out;
    const FRacerBattleState& State = GetState(bPlayer);

    for (int32 i = 0; i < State.ActiveTimedEffects.Num(); ++i)
    {
        const FPerkEffectData& Effect = State.ActiveTimedEffects[i];
        const float Elapsed = State.TimedEffectElapsed.IsValidIndex(i)
                            ? State.TimedEffectElapsed[i] : 0.0f;

        // Only include effects that haven't expired yet
        // (Duration 0 means last the full race)
        if (Effect.Duration <= 0.0f || Elapsed < Effect.Duration)
        {
            // Find the perk this effect belongs to for its display name
            for (UPerkData* Perk : AllPerks)
            {
                if (!Perk) { continue; }
                for (const FPerkEffectData& E : Perk->GetAllEffects())
                {
                    if (E.EffectType == Effect.EffectType &&
                        FMath::IsNearlyEqual(E.Magnitude, Effect.Magnitude) &&
                        FMath::IsNearlyEqual(E.Duration, Effect.Duration))
                    {
                        Out.Add(Perk->DisplayName);
                        break;
                    }
                }
            }
        }
    }
    return Out;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void URaceBattleManager::InitialiseRacerState(FRacerBattleState& OutState,
                                               const FText& Name,
                                               const FDriverStatBlock& ResolvedStats,
                                               UCharacterPerkState* PerkState,
                                               const TArray<UPerkData*>& AllPerks,
                                               UObject* EffectOuter)
{
    OutState = FRacerBattleState();
    OutState.DisplayName    = Name;
    OutState.ResolvedStats  = ResolvedStats;
    OutState.MaxHealth      = static_cast<float>(ResolvedStats.Health);
    OutState.CurrentHealth  = OutState.MaxHealth;

    if (!PerkState) { return; }

    OutState.EquippedSkillPerkIDs = PerkState->EquippedSkillPerkIDs;
    OutState.PassiveEffects       = PerkState->GetAllPassiveEffects(AllPerks);

    // Populate timed effects and initialise their elapsed timers
    OutState.ActiveTimedEffects = PerkState->GetAllActiveEffects(AllPerks);
    OutState.TimedEffectElapsed.Init(0.0f, OutState.ActiveTimedEffects.Num());

    // Instantiate and apply custom Blueprint effect objects
    for (const TSoftClassPtr<UPerkSkillEffect>& SoftClass :
         PerkState->GetAllCustomEffectClasses(AllPerks))
    {
        UClass* Class = SoftClass.LoadSynchronous();
        if (!Class) { continue; }

        UPerkSkillEffect* EffectObj = NewObject<UPerkSkillEffect>(EffectOuter, Class);
        if (EffectObj)
        {
            // Use a blank FPerkEffectData as the data carrier for custom effects;
            // the Blueprint implementation reads any needed data from its asset directly.
            EffectObj->ApplyEffect(nullptr, FPerkEffectData());
            OutState.LiveCustomEffects.Add(EffectObj);
        }
    }
}

void URaceBattleManager::ModifyHealth(FRacerBattleState& State, bool bIsPlayer, float Delta)
{
    const float Previous = State.CurrentHealth;
    State.CurrentHealth = FMath::Clamp(State.CurrentHealth + Delta, 0.0f, State.MaxHealth);

    if (!FMath::IsNearlyEqual(State.CurrentHealth, Previous))
    {
        OnHealthChanged.Broadcast(bIsPlayer, State.CurrentHealth, State.MaxHealth);

        if (State.CurrentHealth <= 0.0f)
        {
            OnRacerDefeated.Broadcast(bIsPlayer);
        }
    }
}

void URaceBattleManager::TickTimedEffects(FRacerBattleState& State, bool bIsPlayer,
                                           float DeltaSeconds)
{
    for (int32 i = State.ActiveTimedEffects.Num() - 1; i >= 0; --i)
    {
        const FPerkEffectData& Effect = State.ActiveTimedEffects[i];

        // Duration 0 means last the full race — never expire
        if (Effect.Duration <= 0.0f) { continue; }

        State.TimedEffectElapsed[i] += DeltaSeconds;

        if (State.TimedEffectElapsed[i] >= Effect.Duration)
        {
            const FPerkEffectData Expired = State.ActiveTimedEffects[i];
            State.ActiveTimedEffects.RemoveAt(i);
            State.TimedEffectElapsed.RemoveAt(i);
            OnTimedEffectExpired.Broadcast(bIsPlayer, Expired);
        }
    }
}
