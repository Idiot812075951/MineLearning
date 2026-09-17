# Combat Animation Timing

Timing ranges below are examples at 30 FPS. The approved character references and gameplay timing contract take priority; skill use does not authorize changing game code or attack windows.

## Attack Phases

| Phase | Frames (30fps) | Description |
|-------|----------------|-------------|
| Anticipation | 4–8 | Wind-up, weight shift |
| Action | 2–4 | Strike — fastest phase |
| Follow-through | 4–8 | Momentum continuation |
| Recovery | 8–16 | Return to idle |

Fast strikes often have a short action phase; holds, grabs and charged releases can differ. Anticipation, impact and recovery should express the chosen attack.

## Hit Reaction

- 2–4 frames impact pose
- 8–12 frames stagger/recoil
- Snap to impact for game feel; smooth for cinematic

## Combo Timing

| Combo Hit | Total Frames |
|-----------|--------------|
| Light 1 | 20–24 |
| Light 2 | 18–22 |
| Heavy | 30–40 |

A cancel window at 60% is one possible design choice, not a requirement. Preserve existing cancel/impact windows unless gameplay changes are explicitly requested.

## Root Motion

Document per animation:
- **In-place:** Root locked; engine applies movement
- **Root motion:** Only when approved, define and bake the intended trajectory onto the agreed root while retaining relative pelvis motion; do not copy all hip bob/sway onto root

## Silhouette Readability

Exaggerate weapon arc during action phase.
Freeze-frame test: silhouette must read attack direction.
Also inspect continuous playback: contact stability, force through chest/pelvis, unclipped weapon paths and recovery. On Guren, check palm normal toward the intended target, elbow plane, wrist separation/rejoin and claw clearance rather than facing the palm toward the camera in every action.

## Game Export

- No foot sliding on in-place attacks
- Loop recovery to idle seamlessly
- Naming example for new assets: `AN_Char_Attack_Light_01`; preserve Guren's existing `Anim_*` names
