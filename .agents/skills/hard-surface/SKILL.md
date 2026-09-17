---
name: hard-surface
description: Plan, model, or review Blender hard-surface forms for MineLearning using reference-led, non-destructive boolean, bevel, panel, and mechanical workflows. Use for isolated armor, props, machines, and high-poly surface work; not for rigging, animation, UV/baking, rendering, or unscoped production-model rewrites.
license: MIT
metadata:
  author: https://github.com/arjun988
  version: "1.0.0"
  upstream-commit: 8f778d2405a214b508d4c7d80742be8e43acdd52
  project-adaptation: "2026-09-06"
  domain: blender
  role: specialist
  triggers: hard surface, sci-fi, industrial, military, vehicle, spaceship, weapon, robot, machinery, boolean, bevel, panel line, greeble, mechanical
  related-skills: mining-game-3d-asset-production, blender-modeling-modifiers
---

# Hard Surface Artist

Production hard surface modeling with manufacturing logic. Readable surfaces. Clean modifier stacks. MCP-first.

## MineLearning Override — Read First

Use this community method under `mining-game-3d-asset-production`, the current Asset Brief, visible references, and the user's current scope. This skill does not authorize edits by itself.

- Confirm the connected Blender version, absolute `.blend` path, dirty state, exact target, and rollback copy before any write.
- Inspect parenting, Armature modifiers, vertex groups, shape keys, UVs, material slots, shared mesh users, transforms, and the current modifier stack.
- Preserve accepted head, chest, and whole-body proportions. Do not change rig, animation, Rest Pose, pivots, parenting, bindings, UVs, material-slot order, or export structure in a modeling-only task.
- Test a new high-poly method on an isolated duplicate outside the production model. The next approved target is a shoulder-armor copy only; do not propagate to the right arm, legs, or feet until that gate passes.
- Keep the project's current Blender MCP. If connection or file identity is uncertain, stop asset writes; do not migrate to another add-on, server, background writer, or UI path.
- Subdivision, Bevel, and Weighted Normal are per-object options, never a mandatory full-body stack. Do not Apply Transform, Remesh, Decimate, auto-fill holes, or apply all modifiers on a bound production mesh.
- Bevel widths, segments, thresholds, bake distances, and polygon budgets come from the current object, scale, camera distance, and Brief—not the example values below.
- Do not add screws, vents, panel lines, or decorative mechanisms without reference or functional support.

## When to Use

- Sci-fi consoles, spaceships, industrial equipment
- Weapons, vehicles, robotics, military props
- Mechanical assemblies with panels, vents, bolts
- Any asset requiring crisp edges, booleans, chamfers

## Design Principles

1. **Primary forms first** — Silhouette reads at distance
2. **Manufacturing logic** — Panel breaks follow construction seams
3. **Surface hierarchy** — Primary > Secondary > Tertiary > Micro (greebles)
4. **Functional readability** — Every form suggests purpose
5. **Edge consistency** — Uniform bevel widths per detail tier

## Workflow

```
Reference → Scale Blockout → Primary Volumes → Panel Breaks
    → Deliberate Boolean/Bevel/Shading Choices
    → Greebles (instanced) → UV Planning → Materials
```

## Boolean Workflow

```
1. Block primary forms (no booleans yet)
2. Create cutter objects on separate collection COL_Booleans
3. Inspect operand scale; do not Apply Scale on a bound/protected mesh
4. Boolean Difference/Union — keep LIVE
5. Fix intersection artifacts with manual cleanup if needed
6. Bevel AFTER boolean in modifier stack
7. Apply booleans only at export prep
```

**Boolean rules:**
- Cutter objects named `SM_Cutter_[Feature]`
- Prefer exact solver for mechanical work
- Avoid booleans on curved surfaces without adequate topology
- Duplicate mesh before first boolean as backup

## Bevel Workflow

Choose width and segments from reference, object scale, silhouette, and intended camera distance. The upstream numeric ranges are examples, not MineLearning defaults.

- Use bevel weight or edge marks for selective beveling
- Use Weighted Normal only if inspection shows it improves the selected object's shading
- Never bevel entire mesh uniformly on complex assets

## Panel Lines

Techniques (prefer non-destructive):
1. Inset faces + slight extrude inward
2. Knife cuts on flat panels
3. Boolean with thin cutter boxes
4. Normal map detail for distant panels

Place panel lines at:
- Structural boundaries
- Access hatches, maintenance points
- Material transitions
- Manufacturing seam locations

## Greebles

- Instance small detail meshes; never model unique unless hero
- Collection: `COL_Greebles`
- Keep greebles on separate mesh for easy toggle
- Budget: greebles should not dominate polycount
- Use array + random transform for scatter greebles

## Hard Edge Management

```
Sharp edges → Mark Sharp or Bevel Weight = 1.0
Soft transitions → Bevel Weight = 0.0–0.5
Auto Smooth angle: 30°–45° for hard surface
Weighted Normal modifier: Keep Sharp enabled
```

## Manufacturing Reference

| Real-World Feature | Modeling Approach |
|--------------------|-------------------|
| Sheet metal bend | Bevel + slight inset |
| Weld seam | Thin raised strip or normal map |
| Bolt pattern | Instanced cylinder, hex inset |
| Vent grille | Boolean cut + array |
| Cable routing | Curve + bevel object or geo nodes |
| Rubber gasket | Separate mesh, slight inset channel |

## Vehicle / Spaceship Specifics

> Prefer **vehicle-artist** for cars, ships, aircraft, and mechs. Use this skill when the vehicle is primarily hard-surface sci-fi detailing under a broader HS brief.

- Establish centerline symmetry early (Mirror modifier)
- Flow lines follow aerodynamic/structural logic
- Landing gear, thrusters as separate objects
- LOD: bake panel detail to normal at distance

## Weapon Specifics

- Grip pivot at hand contact center
- Sight line alignment verified
- Separate moving parts (slide, magazine) as objects
- First-person: optimize silhouette for viewmodel

## MCP Integration

Execute only authorized work through the project's existing MCP:
1. Create blockout primitives
2. Apply mirror/array modifiers
3. Execute boolean operations
4. Add only the selected bevel/shading treatment
5. Instance greeble collections
6. Query polycount per phase

## Constraints

### MUST DO
- Maintain manufacturing plausibility
- Keep modifier stack non-destructive until export
- Use instancing for repeated mechanical detail
- Verify silhouette at 45° increments
- Inspect scale and compensate safely before booleans; Apply Scale is not automatic

### MUST NOT DO
- Boolean on unsubdivided curved surfaces without support
- Uniform bevel on entire complex mesh
- Model every bolt uniquely
- Add shading modifiers without a demonstrated need
- Exceed polycount budget with micro-detail

## Reference Guide

| Topic | Reference | Load When |
|-------|-----------|-----------|
| Boolean deep dive | `references/boolean-workflow.md` | Complex cuts |
| Greeble library | `references/greeble-patterns.md` | Detail pass |
| Project production rules | `../mining-game-3d-asset-production/SKILL.md` | Before asset writes |

## Style Integration

| Style | Adjustment |
|-------|------------|
| realistic-style | Full bevel + PBR materials |
| lowpoly-style | Skip micro-bevels; large flat panels |
| horror-style | Industrial wear; asymmetric damage |
| stylized-style | Exaggerated panel breaks; bold bevels |

## Project References and Source

- Reference reconstruction: `references/reference-image-match.md`, then `references/visual-match-checklist.md`.
- API/parameter operations: use `blender-modeling-modifiers` and verify current Blender RNA first.
- Adapted from [arjun988/blender-skills hard-surface](https://github.com/arjun988/blender-skills/tree/8f778d2405a214b508d4c7d80742be8e43acdd52/.claude/skills/hard-surface), upstream version 1.0.0, MIT. MineLearning overrides take precedence over generic upstream defaults.
