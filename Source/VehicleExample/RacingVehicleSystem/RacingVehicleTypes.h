// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "RacingVehicleTypes.generated.h"

// ---------------------------------------------------------------------------
// Drivetrain
// ---------------------------------------------------------------------------

/** Drivetrain layout of the vehicle. Controls which tuning sections are available. */
UENUM(BlueprintType)
enum class EDrivetrainType : uint8
{
    FWD     UMETA(DisplayName = "Front-Wheel Drive"),
    RWD     UMETA(DisplayName = "Rear-Wheel Drive"),
    AWD     UMETA(DisplayName = "All-Wheel Drive"),
};

// ---------------------------------------------------------------------------
// LSD Type
// ---------------------------------------------------------------------------

/** Limited-Slip Differential engagement direction. */
UENUM(BlueprintType)
enum class ELSDType : uint8
{
    OneWay              UMETA(DisplayName = "1-Way"),
    OnePointFiveWay     UMETA(DisplayName = "1.5-Way"),
    TwoWay              UMETA(DisplayName = "2-Way"),
};

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

    /** Reduction in braking distance, expressed as a positive fraction (0.0�1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chassis")
    float BrakingEfficiencyBonus = 0.0f;

    /** Reduction in vehicle mass (kg) � positive value removes weight */
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

    /** Stat changes this level provides over stock (or over the previous level � designer's choice, just be consistent). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    FPartStatModifiers StatModifiers;

    /**
     * Transmission only: how many forward gears are available at this level.
     * Ignored for all other part slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transmission")
    int32 GearCount = 0;

    // ----- Tuning unlock gates -----

    /**
     * Minimum installed Suspension level before alignment/suspension/stabilizer
     * tuning options for this part level are shown to the player.
     * Only relevant on Suspension part data; ignored on other slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TuningUnlock")
    int32 SuspensionTuningUnlockLevel = 0;

    /**
     * Minimum installed Tire level before tire-width tuning is shown.
     * Only relevant on Tire part data; ignored on other slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TuningUnlock")
    int32 TireTuningUnlockLevel = 0;

    /**
     * Minimum installed Brake level before brake tuning (ABS, balance) is shown.
     * Only relevant on Brake part data; ignored on other slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TuningUnlock")
    int32 BrakeTuningUnlockLevel = 0;

    /**
     * Minimum installed LSD level before LSD tuning is shown.
     * Only relevant on LSD part data; ignored on other slots.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TuningUnlock")
    int32 LSDTuningUnlockLevel = 0;
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
     * Evaluated at normalised RPM (0�1) to scale torque output.
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

    /** Final drive (differential) ratio — copied from VehicleDefinition::BaseStats */
    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    float FinalDriveRatio = 2.81f;

    /** Final per-gear ratios after player tuning */
    UPROPERTY(BlueprintReadOnly, Category = "Stats")
    TArray<float> GearRatios;
};

// ===========================================================================
// Tuning Specs  (designer-defined ranges, live in VehicleDefinition)
// ===========================================================================

/**
 * Generic float tuning parameter: default value and player-adjustable min/max.
 * Used throughout the tuning definition structs below.
 */
USTRUCT(BlueprintType)
struct FTuningSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning")
    float Default = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning")
    float Min = -5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tuning")
    float Max = 5.0f;
};

// ---------------------------------------------------------------------------
// Alignment Tuning Definition
// ---------------------------------------------------------------------------

/**
 * Designer-configured ranges for all wheel alignment parameters.
 * Stored on the VehicleDefinition.
 *
 * Unlock gates reference part levels:
 *   - All settings except TireWidth require the installed Suspension part
 *     to be >= MinSuspensionLevelForAlignment.
 *   - TireWidth (front/rear) requires the installed Tire part
 *     to be >= MinTireLevelForTireWidth.
 */
USTRUCT(BlueprintType)
struct FAlignmentTuningDef
{
    GENERATED_BODY()

    /** Minimum installed Suspension level to unlock all alignment tuning (camber, toe, ride height, offset). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    int32 MinSuspensionLevelForAlignment = 1;

    /** Minimum installed Tire level to unlock tire-width tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    int32 MinTireLevelForTireWidth = 1;

    // Camber  (-10 to +10)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camber")
    FTuningSpec CamberFront = { 0.0f, -10.0f, 10.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camber")
    FTuningSpec CamberRear = { 0.0f, -10.0f, 10.0f };

    // Toe  (-5 to +5)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toe")
    FTuningSpec ToeFront = { 0.0f, -5.0f, 5.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toe")
    FTuningSpec ToeRear = { 0.0f, -5.0f, 5.0f };

    // Ride Height  (-5 to +5)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RideHeight")
    FTuningSpec RideHeightFront = { 0.0f, -5.0f, 5.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RideHeight")
    FTuningSpec RideHeightRear = { 0.0f, -5.0f, 5.0f };

    // Offset  (0 to +10)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offset")
    FTuningSpec OffsetFront = { 0.0f, 0.0f, 10.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offset")
    FTuningSpec OffsetRear = { 0.0f, 0.0f, 10.0f };

    /**
     * Tire Width offset.  The slider runs from 60 (narrowest) to 0 (widest).
     * Default 0 = widest/stock.  Increasing the value narrows the tire.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TireWidth")
    FTuningSpec TireWidthFront = { 0.0f, 0.0f, 60.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TireWidth")
    FTuningSpec TireWidthRear = { 0.0f, 0.0f, 60.0f };
};

// ---------------------------------------------------------------------------
// Brake Tuning Definition
// ---------------------------------------------------------------------------

/**
 * Designer-configured brake tuning options.
 * Requires Brake part >= MinBrakeLevelForTuning.
 */
USTRUCT(BlueprintType)
struct FBrakeTuningDef
{
    GENERATED_BODY()

    /** Minimum installed Brake level to unlock brake tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    int32 MinBrakeLevelForTuning = 1;

    /** Whether ABS can be toggled at all on this vehicle. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ABS")
    bool bABSAvailable = true;

    /**
     * Brake balance: negative = more rear bias, 0 = neutral.
     * Range -10 to 0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Balance")
    FTuningSpec BrakeBalance = { 0.0f, -10.0f, 0.0f };
};

// ---------------------------------------------------------------------------
// LSD Tuning Definition  (one per differential unit)
// ---------------------------------------------------------------------------

/**
 * Designer-configured LSD tuning for one differential (front or rear).
 * On AWD vehicles the VehicleDefinition carries both FrontLSD and RearLSD.
 * On FWD/RWD only the applicable one is used.
 */
USTRUCT(BlueprintType)
struct FLSDTuningDef
{
    GENERATED_BODY()

    /** Minimum installed LSD level to unlock LSD tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    int32 MinLSDLevelForTuning = 1;

    /** LSD types the player may select. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LSDType")
    ELSDType DefaultLSDType = ELSDType::OneWay;

    /** Initial torque range (0�10). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LSD")
    FTuningSpec InitialTorque = { 5.0f, 0.0f, 10.0f };

    /** LSD ratio range (0�10). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LSD")
    FTuningSpec LSDRatio = { 5.0f, 0.0f, 10.0f };
};

// ---------------------------------------------------------------------------
// Suspension Tuning Definition
// ---------------------------------------------------------------------------

/**
 * Designer-configured ranges for suspension settings.
 * Requires Suspension part >= MinSuspensionLevelForTuning.
 */
USTRUCT(BlueprintType)
struct FSuspensionTuningDef
{
    GENERATED_BODY()

    /** Minimum installed Suspension level to unlock suspension tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    int32 MinSuspensionLevelForTuning = 1;

    // Spring Rate  (-5 to +5)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpringRate")
    FTuningSpec SpringRateFront = { 0.0f, -5.0f, 5.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpringRate")
    FTuningSpec SpringRateRear = { 0.0f, -5.0f, 5.0f };

    // Damper  (-15 to +15)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damper")
    FTuningSpec DamperFront = { 0.0f, -15.0f, 15.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damper")
    FTuningSpec DamperRear = { 0.0f, -15.0f, 15.0f };

    // Damper Balance  (0 to 100)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damper")
    FTuningSpec DamperBalance = { 50.0f, 0.0f, 100.0f };
};

// ---------------------------------------------------------------------------
// Stabilizer Tuning Definition
// ---------------------------------------------------------------------------

/**
 * Designer-configured stabilizer (anti-roll bar) ranges.
 * Unlocked by the same Suspension part level gate as suspension tuning.
 * Uses the same MinSuspensionLevelForTuning field from FSuspensionTuningDef �
 * the designer sets one threshold that covers both suspension and stabilizer.
 */
USTRUCT(BlueprintType)
struct FStabilizerTuningDef
{
    GENERATED_BODY()

    // Front/Rear stabilizer stiffness offset  (-5 to +5)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stabilizer")
    FTuningSpec StabilizerFront = { 0.0f, -5.0f, 5.0f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stabilizer")
    FTuningSpec StabilizerRear = { 0.0f, -5.0f, 5.0f };
};

// ---------------------------------------------------------------------------
// Torque Balance Definition  (AWD only)
// ---------------------------------------------------------------------------

/**
 * Front-to-rear torque split tuning for AWD vehicles.
 * FrontBias is a whole-number percentage (0�100); rear is implicit (100 - Front).
 * E.g. FrontBias = 50 means 50:50.  FrontBias = 30 means 30:70.
 */
USTRUCT(BlueprintType)
struct FTorqueBalanceDef
{
    GENERATED_BODY()

    /** Default front torque percentage (0�100). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TorqueBalance",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 DefaultFrontBias = 50;

    /** Minimum front torque percentage the player can set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TorqueBalance",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 MinFrontBias = 0;

    /** Maximum front torque percentage the player can set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TorqueBalance",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 MaxFrontBias = 100;
};

// ===========================================================================
// Tuning State  (player-set values, saved per owned vehicle instance)
// ===========================================================================

/** Player-set alignment values for one owned vehicle. */
USTRUCT(BlueprintType)
struct FAlignmentTuningState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float CamberFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float CamberRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float ToeFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float ToeRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float RideHeightFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float RideHeightRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float OffsetFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float OffsetRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float TireWidthFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Alignment")
    float TireWidthRear = 0.0f;
};

/** Player-set brake tuning for one owned vehicle. */
USTRUCT(BlueprintType)
struct FBrakeTuningState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Brakes")
    bool bABSEnabled = false;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Brakes")
    float BrakeBalance = 0.0f;
};

/** Player-set LSD tuning for one differential unit. */
USTRUCT(BlueprintType)
struct FLSDTuningState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "LSD")
    ELSDType LSDType = ELSDType::OneWay;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "LSD")
    float InitialTorque = 5.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "LSD")
    float LSDRatio = 5.0f;
};

/** Player-set suspension tuning for one owned vehicle. */
USTRUCT(BlueprintType)
struct FSuspensionTuningState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Suspension")
    float SpringRateFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Suspension")
    float SpringRateRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Suspension")
    float DamperFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Suspension")
    float DamperRear = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Suspension")
    float DamperBalance = 50.0f;
};

/** Player-set stabilizer tuning for one owned vehicle. */
USTRUCT(BlueprintType)
struct FStabilizerTuningState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Stabilizer")
    float StabilizerFront = 0.0f;

    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Stabilizer")
    float StabilizerRear = 0.0f;
};

/**
 * Player-set torque balance for one AWD owned vehicle.
 * FrontBias is a whole-number percentage (0�100); rear is (100 - FrontBias).
 */
USTRUCT(BlueprintType)
struct FTorqueBalanceState
{
    GENERATED_BODY()

    /** Front torque percentage.  Rear is implicitly (100 - FrontBias). */
    UPROPERTY(SaveGame, BlueprintReadWrite, Category = "TorqueBalance",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 FrontBias = 50;
};
