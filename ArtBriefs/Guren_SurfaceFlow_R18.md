# 红莲 R18 曲面与哑光漆面修复

Status: Implemented and verified in the UE test level, 2026-09-14. Current visual source: `V027A_R18_SurfaceFlow.blend`. Delivery evidence: `Docs/Guren_SurfaceFlow_R18_Delivery.md`.

Current scope authorized by the user's 2026-09-14 feedback: eliminate blocky major-plane transitions visible in the marked shoulders, pelvis front, knees, forearm guard, backpack, rear waist and thighs. R17 fixed fragmented highlights but retained excessive planar ridges. R18 must change local surface form, then use a coherent matte/satin paint in the UE level.

Source of truth: live `V027A_R17_VisualPolish.blend`, preserved before edits; new working source `V027A_R18_SurfaceFlow.blend`. Preserve the R17 chest optical coverage, energy-wing design, original character identity, dimensions, head identity, rig/Rest matrices/Actions/constraints/drivers, all part pivots/transforms/bone parents, unrelated UVs, weights and material slots. User explicitly authorizes local armor geometry and modifier changes to reduce blockiness. Do not alter gameplay, camera controls or animation data.

Method: prove a rounded-surface treatment on isolated shoulder duplicates under reflective gray material and narrow rectangular light sources. Inspect front, oblique, side and back. Widen and smooth selected major transitions rather than only micro-bevel the edges. Preserve important armor contours and articulated clearances. Propagate only the inspected method, with per-part settings. Topology edits are limited to the identified rigid armor shells if modifiers cannot give clean flow; no whole-body remesh or topology rewrite.

Verification: identical diagnostic material/light/camera before-after; reflections must transition continuously without unwanted planar steps, pinching or wobble. High-reflection stress setup must remain separately available in Blender. Final painted material is authored in Blender and mapped traceably into UE. Check existing test level under ordinary lighting and original pose samples; preserve Skeleton, material slots, AnimBP references and source normals. Back up affected assets before reimport; retain R17.
