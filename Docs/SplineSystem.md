# NPC Spline System — Designer Reference

## Overview

The spline system gives NPCs a set of named, lane-indexed paths to follow during patrol and racing.  
It has two layers:

| Layer | Class | Purpose |
|-------|-------|---------|
| **Lane component** | `URacingSplineComponent` | One spline = one lane on a road. Carries a `LaneIndex` and query helpers. |
| **Lane group actor** | `ACourseSplineActor` | Container for all lanes on one road. Placed in the level. |

The AI controller (`ARacingAIController`) keeps a `CurrentLaneSpline` and blends to a new one whenever obstacle avoidance triggers a lane change.  A separate read-only **RacingSpline** is used only for distance-to-player calculations and corner braking — it is never used for steering.

---

## Setting Up a Road in the Editor

### 1. Place a `CourseSplineActor`

- In the **Place Actors** panel (or the Outliner context menu) add a `BP_CourseSplineActor` (or the native C++ class) to the level.
- Rename it to something descriptive, e.g. `CSA_MainLoop`.

### 2. Add lane splines

- Select the actor in the Outliner.
- In the **Details → Add Component** search for `RacingSplineComponent` and add one per lane.
- Rename each component clearly, e.g. `Lane_Left`, `Lane_Center`, `Lane_Right`.

### 3. Set `LaneIndex` on each component

Open the Details panel for each `URacingSplineComponent` and set **Lane → Lane Index**:

| Value | Meaning |
|-------|---------|
| `0` | Leftmost lane (relative to the spline's travel direction) |
| `1` | Centre lane |
| `2` | Rightmost lane |
| … | Increasing rightward |

The values only need to be **consistent and unique within one actor** — the AI uses them as a tiebreaker when two sibling lanes are equally clear (it prefers the higher / rightmost index, matching real-world "keep right" driving).

### 4. Draw the spline points

- Select a `RacingSplineComponent` in the viewport.
- Drag spline handles along the centre of the lane.
- Enable **Closed Loop** on each spline for circuits (Details → Spline → Closed Loop = true).
- Repeat for every lane.

> **Important:** Each lane spline must follow the **same direction** (clockwise or counter-clockwise) so that the AI's "nearest point" projection gives consistent results across siblings.

---

## Assigning a CourseSplineActor to an NPC

On the `ANPCPatrolActor` Blueprint (or Details panel), set:

| Property | Value |
|----------|-------|
| **Patrol Spline** | The specific `URacingSplineComponent` (e.g. `CSA_MainLoop → Lane_Right`) the NPC should start on |

The NPC's `CurrentLaneSpline` is initialised to this component at race/idle start.  Because the component lives on a `CourseSplineActor`, the AI can call `GetAllSplines()` on the owner to find every sibling lane whenever it needs to change lanes.

---

## Runtime Behaviour

### Normal driving

Each tick the AI queries `CurrentLaneSpline` at a lookahead distance (`SteeringLookaheadCm`, default 900 cm) and aligns its heading to the spline tangent at that point. A small lateral-correction term gently nudges the car back to the lane centre when it drifts.

### Lane change triggered by obstacle detection

Every tick `EvaluateLaneChange()` sweeps a sphere (`ObstacleSweepRadiusCm` radius, `ObstacleLookaheadCm` length) forward. If a vehicle pawn is detected ahead:

1. Each sibling lane on the same `CourseSplineActor` is swept with the same capsule.
2. The sibling with the **fewest forward obstacles** is chosen. Ties go to the highest `LaneIndex`.
3. If the winner is truly clearer than the current lane, `PreviousLaneSpline` is set and `CurrentLaneSpline` is switched.

### Smooth blend

`TickLaneBlend()` advances `LaneBlendAlpha` from 0 → 1 over `LaneChangeDurationSec` seconds. While the alpha is < 1, `ComputeSplineSteeringInput()` lerps the steering result between the old and new lane so the path change is smooth.

---

## Tunable Parameters

All values are `UPROPERTY(EditAnywhere)` and can be changed per-Blueprint in Class Defaults without recompiling.

### Steering (`FCorneringConfig` — in `FRacingAIConfig`)

| Property | Default | Effect |
|----------|---------|--------|
| `SteeringLookaheadCm` | 900 cm | How far ahead the AI looks for the lane tangent. Higher = smoother corners, slower reaction to tight hairpins. |
| `LateralCorrectionScaleCm` | 600 cm | Lateral offset at which the centering nudge reaches full strength. Increase to tolerate wider drift before correcting. |
| `LateralCorrectionWeight` | 0.25 | Blend weight of the lateral correction into the final steer signal (0–1). Keep low to avoid oscillation. |
| `SplineFollowStrength` | 0.85 | How strongly the heading component steers toward the tangent. |

### Lane change (`FLaneConfig` — in `FRacingAIConfig`)

| Property | Default | Effect |
|----------|---------|--------|
| `LaneChangeDurationSec` | 1.5 s | Duration of the steering blend between old and new lane. |
| `ObstacleLookaheadCm` | 1500 cm | How far ahead to sweep for blocking vehicles. |
| `ObstacleSweepRadiusCm` | 180 cm | Sweep radius. Should be ≈ half a lane width. |

---

## Multi-Route Roads

One `ACourseSplineActor` = one road. If your map has separate routes or forks, use **separate actors**:

```
CSA_MainLoop    — left/center/right lanes of the main circuit
CSA_MountainFork — the mountain shortcut road (two lanes)
CSA_ReturnRoad   — the return leg of a point-to-point layout
```

NPCs assigned to `CSA_MountainFork` will only ever change lanes within that actor's siblings. They will never accidentally jump to `CSA_MainLoop`'s lanes.

---

## Quick-Start Checklist

- [ ] Place a `CourseSplineActor` in the level
- [ ] Add one `URacingSplineComponent` per lane via **+Add Component**
- [ ] Set `LaneIndex` on each component (0 = leftmost)
- [ ] Draw splines along lane centres, all in the same travel direction
- [ ] Enable **Closed Loop** on each spline
- [ ] Assign the desired spline component as the NPC's **Patrol Spline**
- [ ] Optionally tune `FLaneConfig` and `FCorneringConfig` in the Game Mode's Class Defaults
