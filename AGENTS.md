# Agent Instructions — VehicleExample (Unreal Engine 5)

## Build Command

Use this PowerShell command to compile the project and check for errors:

```powershell
Start-Process -FilePath "G:\EpicGames\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
    -ArgumentList "VehicleExampleEditor", "Win64", "Development", `
        "`"C:\Users\Alex\Documents\Unreal Projects\VehicleExample\VehicleExample.uproject`"" `
    -Wait -NoNewWindow `
    -RedirectStandardOutput "$env:TEMP\ue_build_out.txt" `
    -RedirectStandardError  "$env:TEMP\ue_build_err.txt"

Get-Content "$env:TEMP\ue_build_out.txt" | Select-String "error C|error LNK|fatal error|Result:"
```

**Expect** `Result: Succeeded` when the build is clean.  
**Failures** show `error C####:` (compiler) or `error LNK####:` (linker) lines.

## Project Layout

| Folder | Purpose |
|--------|---------|
| `Source/VehicleExample/RacingAISystem/` | Game mode, NPC patrol/AI, spline, HUD widgets |
| `Source/VehicleExample/RacingCharacterSystem/` | Perk system, battle manager, NPC/player data |
| `Source/VehicleExample/RacingVehicleSystem/` | Vehicle inventory, definitions, owned vehicle |
| `Source/VehicleExample/UI/` | Hub menus, vehicle display |

## Key Classes

- **`ACourseGameMode`** — Orchestrates race battles: health floats, collision damage, distance drain, HUD.
- **`ANPCPatrolActor`** — Manages one NPC: spawns pawn, transitions idle↔race, challenge trigger.
- **`ARacingAIController`** — AI state machine; exposes `GetContext().DistanceToPlayerCm` for spline gap.
- **`URaceBattleManager`** — Perk-aware battle state (designed but not yet integrated into CourseGameMode).
- **`SRaceHUDWidget`** — Slate widget showing health bars and elapsed timer during a battle.

## Important Conventions

- Health is tracked as raw `float` locals in `ACourseGameMode` (`PlayerCurrentHP`, `NPCCurrentHP`).  
  Do **not** route through `URaceBattleManager` unless explicitly asked.
- Wall damage detection: `ImpactNormal.Z < 0.7f` (not a road/ground hit) **and** impulse ≥ `MinWallImpulse`.
- Distance drain threshold: `DistanceDrainThresholdCm` (default 2377.44 cm ≈ 26 yards).  
  Sign of `DistanceToPlayerCm`: **positive = player ahead**, **negative = player behind NPC**.
- All tuning values (`CollisionDamageScale`, `WallCollisionDamageScale`, `MinWallImpulse`,  
  `MaxWallDamagePerHit`, `DistanceDrainThresholdCm`, `DistanceDrainRatePerSecond`) are exposed as  
  `UPROPERTY(EditAnywhere)` so they can be tweaked in `BP_CourseGameMode` Class Defaults without recompiling.
- Wall damage is capped per hit by `MaxWallDamagePerHit` (default 15) to prevent single high-speed  
  impacts from being instantly lethal. Chaos physics impulses scale with speed and can be enormous.
