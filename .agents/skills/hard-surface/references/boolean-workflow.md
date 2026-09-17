# Boolean Workflow — MineLearning / Blender 5.1.2

Upstream source: [boolean-workflow.md at 8f778d2](https://github.com/arjun988/blender-skills/blob/8f778d2405a214b508d4c7d80742be8e43acdd52/.claude/skills/hard-surface/references/boolean-workflow.md).

Read-only RNA inspection on 2026-09-06 exposed `FLOAT`, `EXACT`, and `MANIFOLD`. Re-check current RNA before use; do not use the older `FAST` identifier from unrelated examples. Solver choice must follow geometry and test results; `EXACT` is not a universal default.

## Solver Selection

| Solver | Use When |
|--------|----------|
| Float | General floating-point solver; validate on the actual cut |
| Exact | Use when its handling is needed and verified |
| Manifold | Consider only for suitable manifold inputs; verify the result |

## Common Artifacts & Fixes

| Artifact | Fix |
|----------|-----|
| Z-fighting faces | Merge by distance; manual face cleanup |
| Missing faces | Increase cutter overlap; check normals |
| Ngons at intersection | Convert to quads with loop cuts |
| Shading breaks | Inspect topology, normals, smooth-by-angle, and bevel; Weighted Normal is optional |

## Cutter Best Practices

- Cutter extends past target mesh on all axes
- Cutter has clean manifold geometry
- Inspect cutter scale; Apply Scale is not automatic on protected or bound data
- Keep cutters in `COL_Booleans` collection (hidden in render)

## Stack Order

```
Common:   Mirror → Boolean → Bevel → optional shading treatment
Actual:   preserve or revise the verified object-specific stack deliberately
```

## Batch Boolean Strategy

For multiple panel cuts:
1. Combine cutters with Boolean Union first (separate object)
2. Single Boolean Difference against target
3. Reduces stack depth and artifacts
