---
name: blender-animation
description: Plan, author, or review Blender humanoid/mecha locomotion, idle, combat and mechanical animation using reference, blocking, timing, weight, arcs, curve polish and continuous playback. Use for motion quality, not static modeling or automatic rig repair.
license: MIT
metadata:
  author: https://github.com/arjun988
  version: "1.0.0"
  domain: blender
  role: specialist
  triggers: animation, animate, walk cycle, run cycle, idle, combat, NLA, graph editor, keyframe
  related-skills: mining-game-3d-asset-production, blender-animation-rigging
  upstream-commit: 8f778d2405a214b508d4c7d80742be8e43acdd52
---

# Animation

Game-ready motion with clean keyframes. Readable silhouettes in motion. Engine-compatible export.

## MineLearning Integration

Use with `mining-game-3d-asset-production` and its animation rules. The current user request and approved Asset Brief determine scope, visual references and protected data; this supplementary skill does not authorize rig replacement, new animations, root motion, gameplay edits or export.

- Preserve existing Action names, slots, NLA state, production skeleton/Rest and bindings. Guren's existing `Anim_*` / `Pose_*` names are not renamed to the upstream `AN_` example convention.
- For humanoids/mecha, review head visibility, chest/pelvis counterbalance, support/weight transfer, elbow/knee planes, foot contact and hand orientation in motion. For Guren, also validate shoulder-to-wrist continuity, the intentional launch seam/rejoin and independently articulated claws; use the current Brief, not a historical frame number, as authority.
- Improve authoring curves deliberately. Dense linear baked animation is valid; do not blindly convert every baked key to Bezier or remove keys from protected actions. Rigid armor does not squash/stretch like flesh; sell weight through timing and joint rotation.
- Validate actual continuous playback at the intended FPS plus critical contact/impact/launch frames and transitions. A pose sheet alone cannot establish motion quality. Keep technical checks separate from visual approval.
- Use the existing connected Blender MCP and the project's write gate. If connection/path is uncertain, stop asset writes; do not silently switch to a background writer. Installing this skill is not installing a Blender add-on.

## When to Use

- Character locomotion (walk, run, idle)
- Combat animations
- Mechanical/prop animation
- Camera animation
- Object animation with constraints

## Workflow

```
Reference → Blocking → Breakdown → Splining → Polish
    → Continuous Playback / Cycle Validation → Review
    → [Only when requested: selected Action/NLA export workflow]
```

## Locomotion Cycles

| Cycle | Key Poses | Frames (30fps) |
|-------|-----------|----------------|
| Walk | Contact, Down, Pass, Up × 2 | 24–32 |
| Run | Contact, Drive, Flight, Recovery | 16–24 |
| Idle | Breathe, weight shift, blink | 60–120 loop |

Frame counts above are starting examples, not fixed requirements. Match reference timing and the game's approved motion contract. For a new asset without a naming convention, `AN_[Char]_[Action]_[Variant]` is an option; preserve existing project names.

## Animation Principles (Applied)

1. **Timing** — Weight through spacing
2. **Arcs** — Natural limb paths
3. **Overlap** — Secondary motion offset
4. **Anticipation** — Wind-up before action
5. **Exaggeration** — Style-dependent (see style skills)

## Mechanical Animation

- Use constraints over bone deformation
- Hinge: Limit Rotation constraint
- Pistons: use actual rigid telescoping pieces and suitable tracking/copy constraints. Stretch To is only an option for explicitly stretchable auxiliary geometry, never locked production bones or rigid armor.
- Gears: Driver for synchronized rotation
- Doors: Empty pivot + rotation keyframes

## Graph Editor Workflow

```
1. Block stepped keys (Constant interpolation)
2. Splining: shape suitable authoring curves; inspect Bezier handles for overshoot
3. Ease: Weighted handles for acceleration
4. Cycle: Match boundary pose AND velocity; avoid playing a duplicate endpoint twice
5. Clean: Remove demonstrably redundant authoring keys only; recheck playback
```

## Optional NLA Export Workflow

Only when export is explicitly requested and the chosen exporter uses NLA. Direct Action export is also valid. Preserve existing active-action/slot and NLA state; avoid evaluating the same motion twice. Follow the production skill's export checklist before claiming UE-ready.

```
1. Action created and named per convention
2. Push down to NLA strip
3. Strip set to Repeat for cycles
4. Single strip active for export
5. Scale: 1.0; no NLA time remapping unless intentional
```

## Camera Animation

- Smooth Bezier interpolation where appropriate to the intended shot; cuts and intentional linear moves are valid
- Avoid sudden direction changes
- Match focal length to subject scale
- Camera shake: noise modifier on location (subtle)

## MCP Integration

1. Insert keyframes via MCP if supported
2. Query action names and frame ranges
3. Validate bone/object animation data exists pre-export
4. Set frame range via MCP
5. Only if requested, follow the project's animation and UE export checks

## Game-Ready Requirements

- [ ] Root motion decision documented (in-place vs root motion)
- [ ] Loop seam invisible (walk/idle/run)
- [ ] No unintended or non-unit production-bone scale; do not delete approved constant identity channels merely to satisfy a checklist
- [ ] Animation length matches engine state machine
- [ ] Facial animation on separate action layer if needed
- [ ] Frame rate: 30fps default (confirm project spec)

## Constraints

### MUST DO
- Use relevant motion references for locomotion and combat; do not fabricate reference provenance
- Block in stepped mode before splining
- Preserve the current project's Action naming convention
- Test cycle loop before export
- Match animation style to art direction

### MUST NOT DO
- Blindly linearize sparse organic authoring curves or blindly smooth dense baked motion
- Add unapproved scale animation to rigid production bones
- Over-animate idle (subtle is better)
- Export before approval and the selected exporter strategy are established
- Skip root motion documentation

## Reference Guide

| Topic | Reference | Load When |
|-------|-----------|-----------|
| Walk cycle breakdown | `references/walk-cycle.md` | Locomotion |
| Combat timing | `references/combat-animation.md` | Action games |

## Source and Adaptation

Adapted from [arjun988/blender-skills animation](https://github.com/arjun988/blender-skills/tree/8f778d2405a214b508d4c7d80742be8e43acdd52/.claude/skills/animation), MIT (see `LICENSE`). Project adaptation dated 2026-09-05 preserves the workflow while correcting rigid-rig, naming, baked-curve and export assumptions. This is guidance for the agent, not an automatic animation-quality solver.
