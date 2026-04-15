// Copyright Epic Games, Inc. All Rights Reserved.

#include "SportsCarVehicleDefinition.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Helper: build one FPartLevelData entry
// ---------------------------------------------------------------------------

static FPartLevelData MakePartLevel(
    FText Name, FText Desc, int32 Price, int32 PointsReq,
    float PowerHP, float TorqueNm, float WeightKg, float Grip,
    float Brake = 0.f, float Drag = 0.f,
    float NitroCap = 0.f, float NitroForce = 0.f,
    int32 GearCount = 0)
{
    FPartLevelData L;
    L.DisplayName             = Name;
    L.Description             = Desc;
    L.PurchasePrice           = Price;
    L.UnlockPointsRequired    = PointsReq;
    L.StatModifiers.PowerHP   = PowerHP;
    L.StatModifiers.TorqueNm  = TorqueNm;
    L.StatModifiers.WeightReductionKg     = WeightKg;
    L.StatModifiers.GripBonus             = Grip;
    L.StatModifiers.BrakingEfficiencyBonus = Brake;
    L.StatModifiers.DragDelta             = Drag;
    L.StatModifiers.NitroCapacity         = NitroCap;
    L.StatModifiers.NitroForce            = NitroForce;
    L.GearCount = GearCount;
    return L;
}

static FGearRatioSpec MakeGear(float Default, float Min, float Max)
{
    FGearRatioSpec G;
    G.DefaultRatio = Default;
    G.MinRatio     = Min;
    G.MaxRatio     = Max;
    return G;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

USportsCarVehicleDefinition::USportsCarVehicleDefinition()
{
}

void USportsCarVehicleDefinition::PostInitProperties()
{
    Super::PostInitProperties();

    // Only fill defaults when this is a CDO or a freshly created asset
    // (not when being loaded from an existing saved asset).
    if (!HasAnyFlags(RF_ClassDefaultObject)) { return; }

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------
    VehicleID   = FName("SportsCar_01");
    DisplayName = NSLOCTEXT("VehicleDef", "SportsCarName", "Type-SR Sport Coupe");
    Description = NSLOCTEXT("VehicleDef", "SportsCarDesc",
        "A lightweight rear-wheel-drive coupe built for the mountain passes. "
        "Nimble handling and a high-revving inline-four make it a favourite "
        "among street racers looking to prove themselves.");
    PurchasePrice = 280000;

    // -----------------------------------------------------------------------
    // Drivetrain
    // -----------------------------------------------------------------------
    DrivetrainType = EDrivetrainType::RWD;

    // -----------------------------------------------------------------------
    // Base Stats  (stock, before any parts)
    // -----------------------------------------------------------------------
    BaseStats.MassKg            = 1180.f;
    BaseStats.DragCoefficient   = 0.30f;
    BaseStats.ChassisHeight     = 136.f;
    BaseStats.FinalDriveRatio   = 3.90f;
    BaseStats.BaseGripMultiplier = 1.0f;

    // -----------------------------------------------------------------------
    // Engine
    // -----------------------------------------------------------------------
    EngineDefinition.BasePowerHP       = 210.f;
    EngineDefinition.BaseTorqueNm      = 260.f;
    EngineDefinition.MaxRPM            = 8200.f;
    EngineDefinition.IdleRPM           = 850.f;
    EngineDefinition.EngineBrakeEffect = 0.22f;

    // -----------------------------------------------------------------------
    // Default Gear Ratios  (6-speed, close-ratio)
    // -----------------------------------------------------------------------
    DefaultGearRatios.Empty();
    DefaultGearRatios.Add(MakeGear(3.54f, 2.80f, 4.30f)); // 1st
    DefaultGearRatios.Add(MakeGear(2.11f, 1.60f, 2.60f)); // 2nd
    DefaultGearRatios.Add(MakeGear(1.46f, 1.10f, 1.80f)); // 3rd
    DefaultGearRatios.Add(MakeGear(1.10f, 0.85f, 1.40f)); // 4th
    DefaultGearRatios.Add(MakeGear(0.86f, 0.65f, 1.10f)); // 5th
    DefaultGearRatios.Add(MakeGear(0.70f, 0.55f, 0.90f)); // 6th

    // -----------------------------------------------------------------------
    // Part Slots — 4 levels each: Stock (implicit 0) ? Sport ? Super ? Racing
    // Stock is always level 0, so we define levels 1, 2, 3.
    // -----------------------------------------------------------------------

    // --- Power Unit ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_PowerUnit"));
        PD->SlotType = EPartSlot::PowerUnit;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "PowerUnit", "Power Unit");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","PU1","Sport Tune"),
            NSLOCTEXT("Parts","PU1D","ECU remap and intake cleaning. Noticeable mid-range gain."),
            18000, 0, 30.f, 40.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","PU2","High-Comp Kit"),
            NSLOCTEXT("Parts","PU2D","High-compression pistons and polished ports. Strong top-end power."),
            55000, 500, 55.f, 65.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","PU3","Full Race Spec"),
            NSLOCTEXT("Parts","PU3D","Race-prepped internals, forged crank, and individual throttle bodies."),
            120000, 1500, 90.f, 100.f, 0.f, 0.f));
        AvailableParts.Add(EPartSlot::PowerUnit, PD);
    }

    // --- Exhaust ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Exhaust"));
        PD->SlotType = EPartSlot::Exhaust;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Exhaust", "Exhaust");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","EX1","Cat-Back"),
            NSLOCTEXT("Parts","EX1D","Straight-through cat-back exhaust. Light weight, good mid gain."),
            8000, 0, 10.f, 15.f, 5.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","EX2","Headers + Cat-Back"),
            NSLOCTEXT("Parts","EX2D","Performance headers improve scavenging across the rev range."),
            22000, 200, 20.f, 28.f, 8.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","EX3","Race Exhaust"),
            NSLOCTEXT("Parts","EX3D","Full titanium race exhaust. Maximum flow, maximum noise."),
            48000, 800, 30.f, 38.f, 12.f, -0.01f));
        AvailableParts.Add(EPartSlot::Exhaust, PD);
    }

    // --- Intake ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Intake"));
        PD->SlotType = EPartSlot::Intake;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Intake", "Intake");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","IN1","Cold Air Intake"),
            NSLOCTEXT("Parts","IN1D","Draws cooler, denser air. Easy top-end improvement."),
            6000, 0, 8.f, 10.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","IN2","High-Flow Intake"),
            NSLOCTEXT("Parts","IN2D","Large-bore intake pipe and high-flow filter."),
            18000, 100, 15.f, 18.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","IN3","ITBs"),
            NSLOCTEXT("Parts","IN3D","Individual throttle bodies — screaming response at high RPM."),
            42000, 600, 22.f, 26.f, 0.f, 0.f));
        AvailableParts.Add(EPartSlot::Intake, PD);
    }

    // --- Brake ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Brake"));
        PD->SlotType = EPartSlot::Brake;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Brake", "Brakes");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BR1","Sport Pads"),
            NSLOCTEXT("Parts","BR1D","High-friction pads with better fade resistance."),
            9000, 0, 0.f, 0.f, 0.f, 0.08f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BR2","Cross-Drilled Rotors"),
            NSLOCTEXT("Parts","BR2D","Drilled rotors and sport calipers. Improved heat dissipation."),
            28000, 300, 0.f, 0.f, 5.f, 0.15f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BR3","Big Brake Kit"),
            NSLOCTEXT("Parts","BR3D","6-piston monobloc calipers and two-piece rotors."),
            65000, 1000, 0.f, 0.f, 10.f, 0.22f));
        AvailableParts.Add(EPartSlot::Brake, PD);
    }

    // --- Clutch ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Clutch"));
        PD->SlotType = EPartSlot::Clutch;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Clutch", "Clutch");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","CL1","Sport Clutch"),
            NSLOCTEXT("Parts","CL1D","Higher clamping force. Handles extra power without slip."),
            7500, 0, 0.f, 0.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","CL2","Twin-Plate"),
            NSLOCTEXT("Parts","CL2D","Lightweight twin-plate ceramic clutch. Fast engagement."),
            24000, 400, 0.f, 0.f, 0.f, 0.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","CL3","Race Clutch"),
            NSLOCTEXT("Parts","CL3D","Full competition clutch. Brutal engagement, enormous capacity."),
            55000, 1200, 0.f, 0.f, 0.f, 0.f));
        AvailableParts.Add(EPartSlot::Clutch, PD);
    }

    // --- LSD ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_LSD"));
        PD->SlotType = EPartSlot::LSD;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "LSD", "LSD");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","LS1","1-Way LSD"),
            NSLOCTEXT("Parts","LS1D","Basic torque-sensing LSD. Tightens corner exit stability."),
            12000, 0, 0.f, 0.f, 0.f, 0.04f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","LS2","1.5-Way LSD"),
            NSLOCTEXT("Parts","LS2D","Aggressive 1.5-way locking action. Excellent mid-corner."),
            35000, 600, 0.f, 0.f, 0.f, 0.08f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","LS3","2-Way LSD"),
            NSLOCTEXT("Parts","LS3D","Full 2-way: locks on both accel and decel. Maximum rotation."),
            70000, 1400, 0.f, 0.f, 0.f, 0.12f));
        AvailableParts.Add(EPartSlot::LSD, PD);
    }

    // --- Suspension ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Suspension"));
        PD->SlotType = EPartSlot::Suspension;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Suspension", "Suspension");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","SU1","Sport Springs"),
            NSLOCTEXT("Parts","SU1D","Lowered sport springs with uprated dampers. Cleaner cornering."),
            14000, 0, 0.f, 0.f, 8.f, 0.06f));
        PD->Levels[0].SuspensionTuningUnlockLevel = 1;
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","SU2","Coilovers"),
            NSLOCTEXT("Parts","SU2D","Fully adjustable coilovers. Dial in your ideal setup."),
            38000, 700, 0.f, 0.f, 15.f, 0.11f));
        PD->Levels[1].SuspensionTuningUnlockLevel = 1;
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","SU3","Race Suspension"),
            NSLOCTEXT("Parts","SU3D","Motorsport-derived double-wishbone geometry overhaul."),
            85000, 1600, 0.f, 0.f, 20.f, 0.16f));
        PD->Levels[2].SuspensionTuningUnlockLevel = 1;
        AvailableParts.Add(EPartSlot::Suspension, PD);
    }

    // --- Transmission ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Transmission"));
        PD->SlotType = EPartSlot::Transmission;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Transmission", "Transmission");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TR1","Close-Ratio 6-Spd"),
            NSLOCTEXT("Parts","TR1D","Closer gear spacing. Keeps the engine on the boil."),
            16000, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 6));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TR2","Sequential 6-Spd"),
            NSLOCTEXT("Parts","TR2D","Sequential dog-ring gearbox. Faster shifts."),
            45000, 900, 0.f, 0.f, 3.f, 0.f, 0.f, 0.f, 6));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TR3","Race Dog Box"),
            NSLOCTEXT("Parts","TR3D","Full motorsport dog-engagement box. Instant shifts, no synchros."),
            95000, 2000, 0.f, 0.f, 5.f, 0.f, 0.f, 0.f, 6));
        AvailableParts.Add(EPartSlot::Transmission, PD);
    }

    // --- Body ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Body"));
        PD->SlotType = EPartSlot::Body;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Body", "Body");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BO1","Aero Kit"),
            NSLOCTEXT("Parts","BO1D","Front lip and rear spoiler. Reduced drag, improved high-speed stability."),
            20000, 0, 0.f, 0.f, 0.f, 0.03f, 0.f, -0.02f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BO2","Full Wide-Body"),
            NSLOCTEXT("Parts","BO2D","Widened arches, hood vents, and GT wing. Significant downforce."),
            55000, 800, 0.f, 0.f, 0.f, 0.07f, 0.f, -0.04f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","BO3","Carbon Race Shell"),
            NSLOCTEXT("Parts","BO3D","Full carbon fibre body panels. Dramatic weight saving."),
            110000, 2000, 0.f, 0.f, 30.f, 0.10f, 0.f, -0.06f));
        AvailableParts.Add(EPartSlot::Body, PD);
    }

    // --- Tire ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Tire"));
        PD->SlotType = EPartSlot::Tire;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Tire", "Tires");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TI1","Sport Compound"),
            NSLOCTEXT("Parts","TI1D","High-silica sport tyre. More grip in all conditions."),
            11000, 0, 0.f, 0.f, 0.f, 0.07f));
        PD->Levels[0].TireTuningUnlockLevel = 1;
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TI2","Semi-Slick"),
            NSLOCTEXT("Parts","TI2D","Street-legal semi-slick compound. Outstanding dry grip."),
            30000, 500, 0.f, 0.f, 0.f, 0.13f));
        PD->Levels[1].TireTuningUnlockLevel = 1;
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","TI3","Slick"),
            NSLOCTEXT("Parts","TI3D","Full slick racing tyre. Maximum mechanical grip on dry tarmac."),
            65000, 1500, 0.f, 0.f, 0.f, 0.20f));
        PD->Levels[2].TireTuningUnlockLevel = 1;
        AvailableParts.Add(EPartSlot::Tire, PD);
    }

    // --- Nitro ---
    {
        UVehiclePartData* PD = NewObject<UVehiclePartData>(this, TEXT("PartData_Nitro"));
        PD->SlotType = EPartSlot::Nitro;
        PD->SlotDisplayName = NSLOCTEXT("Parts", "Nitro", "Nitro System");
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","NI1","Wet Shot"),
            NSLOCTEXT("Parts","NI1D","Single-stage wet nitrous kit. Short but potent boost."),
            15000, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0, 2.5f, 35000.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","NI2","Dry Shot"),
            NSLOCTEXT("Parts","NI2D","Dry-plate system with larger capacity bottle."),
            38000, 700, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0, 4.0f, 55000.f));
        PD->Levels.Add(MakePartLevel(
            NSLOCTEXT("Parts","NI3","Stage 3 System"),
            NSLOCTEXT("Parts","NI3D","High-flow nozzles and twin bottles. Sustained thrust."),
            80000, 1800, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0, 6.0f, 80000.f));
        AvailableParts.Add(EPartSlot::Nitro, PD);
    }

    // -----------------------------------------------------------------------
    // Tuning Definitions  (FTuningSpec fields are { Default, Min, Max })
    // -----------------------------------------------------------------------

    // Alignment
    AlignmentTuning.CamberFront     = { -1.5f, -3.5f,  0.5f };
    AlignmentTuning.CamberRear      = { -1.0f, -3.0f,  0.5f };
    AlignmentTuning.ToeFront        = {  0.0f, -2.0f,  2.0f };
    AlignmentTuning.ToeRear         = {  0.1f, -2.0f,  2.0f };
    AlignmentTuning.RideHeightFront = {  0.0f, -30.f,  10.f };
    AlignmentTuning.RideHeightRear  = {  0.0f, -30.f,  10.f };

    // Brakes
    BrakeTuning.bABSAvailable = true;
    BrakeTuning.BrakeBalance  = { 55.f, 45.f, 70.f };  // % front

    // Rear LSD
    RearLSDTuning.InitialTorque = {  5.f,  0.f, 80.f };
    RearLSDTuning.LSDRatio      = {  5.f,  0.f, 10.f };

    // Suspension
    SuspensionTuning.SpringRateFront = { 0.0f, -5.0f,  5.0f };
    SuspensionTuning.SpringRateRear  = { 0.0f, -5.0f,  5.0f };
    SuspensionTuning.DamperFront     = { 0.0f, -8.0f,  8.0f };
    SuspensionTuning.DamperRear      = { 0.0f, -8.0f,  8.0f };
    SuspensionTuning.DamperBalance   = { 50.f,  35.f,  65.f };

    // Stabilizer
    StabilizerTuning.StabilizerFront = { 0.0f, -5.0f, 5.0f };
    StabilizerTuning.StabilizerRear  = { 0.0f, -5.0f, 5.0f };
}
