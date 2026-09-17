# Greeble Patterns

Upstream source: [greeble-patterns.md at 8f778d2](https://github.com/arjun988/blender-skills/blob/8f778d2405a214b508d4c7d80742be8e43acdd52/.claude/skills/hard-surface/references/greeble-patterns.md).

MineLearning override: these are examples, not a detail quota or triangle budget. Do not add a bolt, vent, cable port, LED, hinge, rail, scatter system, or other decoration unless the current reference or asset function supports it. Preserve accepted silhouette/proportions and keep experiments isolated and reversible. Do not start UV, bake, LOD, Geometry Nodes scatter, or export work merely because this document mentions them.

## Reusable Greeble Library

Build once, instance everywhere:

| Greeble | Approx Tris | Use |
|---------|-------------|-----|
| Bolt head (hex) | 50 | Surface attachment |
| Vent slot | 100 | Heat dissipation |
| Cable port | 150 | Connectivity |
| LED strip | 80 | Status indicators |
| Hinge bracket | 200 | Mechanical joints |
| Rail segment | 300 | Mounting systems |

## Scatter Rules

- Random rotation on Z only for surface-attached greebles
- Align to face normal using geometry nodes or shrinkwrap
- Density falloff: more greebles near hero camera angles
- Disable greebles on LOD1+

## Collection Structure

```
COL_Greebles
├── SM_Greeble_Bolt_01
├── SM_Greeble_Vent_01
└── SM_Greeble_Port_01
```

Instance via collection instance or geometry nodes scatter.
