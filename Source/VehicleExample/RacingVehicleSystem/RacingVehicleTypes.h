// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "RacingVehicleTypes.generated.h"

// ---------------------------------------------------------------------------
// Part Slots
// ---------------------------------------------------------------------------

/** Every upgradeable slot on a vehicle. Order matches display order in UI. */
UENUM(BlueprintType)
enum class EPartSlot : uint8
{
    PowerUnit       UMETA(DisplayName = "Power Unit"),
    Exhaust         UMETA(DisplayName = "Exhaust"),
    Intake          UMETA(DisplayName = "Intake"),
    Brake           UMETA(DisplayName = "Brake"),
    Clutch          UMETA(DisplayName = "Clutch"),
    LSD             UMETA(DisplayName = "LSD"),
    Suspension      UMETA(DisplayName = "Suspension"),
    Transmission    UMETA(DisplayName = "Transmission"),
    Body            UMETA(DisplayName = "Body"),
    Tire            UMETA(DisplayName = "Tire"),
    Nitro           UMETA(DisplayName = "Nitro System"),
};

// ---------------------------------------------------------------------------
// Stat Modifiers
// Additive deltas applied on top of the vehicle's base stats per installed part.
// ---------------------------------------------------------------------------

/**
 * Flat stat deltas that a single part level contributes.
 * All values are additive on top of the vehicle base stats.
 * Leave a field at 0.0 if a given slot does not affect it.
 */
USTRUCT(BlueprintType)
struct FPartStatModifiers
{
    GENERATED_BODY()

    /** Added to peak engine power output (HP) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
    float PowerHP = 0.0f;

    /** Added to peak engine torque (Nm) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
    float TorqueNm = 0.0f;

    /** Multiplier on redline RPM (e.g. 0.05 = +5 % RPM ceiling) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power")
    float MaxRPMMultiplier = 0.0f;

    /** Reduction in braking distance, expressed as a positive fraction (0.0–1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float BrakingEfficiencyBonus = 0.0f;

    /** Reduction in vehicle mass (kg) — positive value removes weight */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float WeightReductionKg = 0.0f;

    /** Grip multiplier bonus for tyres/suspension (fraction, e.g. 0.1 = +10 %) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float GripBonus = 0.0f;

    /** Drag coefficient delta (negative = less drag) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float DragDelta = 0.0f;

    /** Maximum nitro capacity added (litres / units, designer-defined scale) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nitro")
    float NitroCapacity = 0.0f;

    /** Nitro thrust force added (N) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nitro")
    float NitroForce = 0.0f;
};

// ---------------------------------------------------------------------------
// Part Level
// ---------------------------------------------------------------------------

/** A single upgrade tier for one part slot. Designers fill this out in the Data Asset. */
USTRUCT(BlueprintType)
struct FPartLevelData
{
    GENERATED_BODY()

    /** Human-readable name shown in the shop / garage UI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /** Flavour text / description shown in UI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText Description;

    /** Cost in in-game currency to purchase and install this level */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 PurchasePrice = 0;

    /**
     * Points the player must have spent (global) before this level becomes
     * available for purchase.  Set to 0 for no requirement.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 UnlockPointsRequired = 0;

    /** Stat changes this level provides over stock (or over the previous level — designer's choice, just be consistent). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    FPartStatModifiers StatModifiers;

    /**
     * Transmission only: how many forward gears are available at this level.
     * Ignored for all other part slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
    int32 GearCount = 0;
};

// ---------------------------------------------------------------------------
// Gear Ratio Spec  (per gear, per vehicle definition)
// ---------------------------------------------------------------------------

/** Min/max tuning range for a single gear's ratio. */
USTRUCT(BlueprintType)
struct FGearRatioSpec
{
    GENERATED_BODY()

    /** Default / stock ratio for this gear */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GearRatio")
    float DefaultRatio = 1.0f;

    /** Minimum ratio the player can dial in (shorter, higher top speed) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GearRatio")
    float MinRatio = 0.5f;

    /** Maximum ratio the player can dial in (longer, stronger acceleration) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GearRatio")
    float MaxRatio = 5.0f;
};

// ---------------------------------------------------------------------------
// Engine Definition
// ---------------------------------------------------------------------------

/** Static engine characteristics baked into the vehicle definition. */
USTRUCT(BlueprintType)
struct FEngineDefinition
{
    GENERATED_BODY()

    /** Stock peak power in horsepower */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    float BasePowerHP = 200.0f;

    /** Stock peak torque in Nm */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    float BaseTorqueNm = 350.0f;

    /** Stock redline RPM */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    float MaxRPM = 7000.0f;

    /** RPM at idle */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    float IdleRPM = 900.0f;

    /** Engine braking coefficient */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    float EngineBrakeEffect = 0.2f;

    /**
     * Optional torque curve asset reference (UCurveFloat).
     * Evaluated at normalised RPM (0–1) to scale torque output.
     * Leave null to use a flat curve.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Engine")
    TSoftObjectPtr<UCurveFloat> TorqueCurve;
};

// ---------------------------------------------------------------------------
// Vehicle Base Stats
// ---------------------------------------------------------------------------

/** Chassis and body characteristics defined per vehicle, before any parts are applied. */
USTRUCT(BlueprintType)
struct FVehicleBaseStats
{
    GENERATED_BODY()

    /** Kerb weight in kg */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float MassKg = 1300.0f;

    /** Aerodynamic drag coefficient (Cd) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float DragCoefficient = 0.31f;

    /** Chassis height (affects centre of gravity) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float ChassisHeight = 144.0f;

    /** Final drive ratio (differential) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drivetrain")
    float FinalDriveRatio = 2.81f;

    /** Base grip multiplier for all four wheels (1.0 = stock) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float BaseGripMultiplier = 1.0f;
};

// ---------------------------------------------------------------------------
// Computed / Effective Stats  (built at runtime from base + installed parts)
// ---------------------------------------------------------------------------

/**
 * The fully-resolved stats for one owned vehicle instance after all installed
 * parts and gear ratio edits have been applied.  Rebuilt whenever a part is
 * changed or gears are adjusted.
 */
USTRUCT(BlueprintType)
struct FEffectiveVehicleStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float PowerHP = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float TorqueNm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float MaxRPM = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float MassKg = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float DragCoefficient = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float GripMultiplier = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float BrakingEfficiency = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float NitroCapacity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float NitroForce = 0.0f;

    /** Final per-gear ratios after player tuning */
    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    TArray<float> GearRatios;
};
