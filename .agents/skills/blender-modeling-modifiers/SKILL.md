---
name: blender-modeling-modifiers
description: Inspect or implement Blender 5.1 modeling and modifier operations for MineLearning with current-RNA verification, context-safe bpy/bmesh usage, and production-mesh protection. Use for modifier stacks, booleans, mesh editing, procedural test geometry, and isolated high-poly experiments; not for automatic cleanup, rigging, animation, rendering, UV/baking, or destructive batch conversion.
license: MIT
metadata:
  author: https://github.com/ra100
  upstream-plugin-version: "1.3.0"
  upstream-commit: 78e9151fdc9e01ce37f1d16a9b677c3047411885
  project-adaptation: "2026-09-06"
  blender-verified: "5.1.2"
---

# Blender Modeling & Modifiers Expert

## MineLearning Scope and Compatibility — Read First

Use with `mining-game-3d-asset-production`; add `hard-surface` for hard-surface shape decisions. This skill supplies technical reference and does not grant permission to alter a production asset.

- Prefer the project's already configured Blender MCP and bundled API/manual search. Do not install or migrate to the upstream plugin/MCP setup, and do not bypass a failed live connection with a background writer.
- Confirm the running version, absolute `.blend` path, dirty state, target, and rollback point before any write. Inspect data users, mode, active/selected state, transforms, parenting, Armature modifiers, vertex groups, shape keys, UVs, material slots, and modifier order.
- Verify every operator, property, and enum against current Blender RNA/docs before executing a reference example.
- Test new methods on an isolated duplicate outside the production model. The next planned validation target is a shoulder-armor copy only; do not propagate to the production model, right arm, legs, or feet without approval.
- Do not apply transforms/all modifiers, Remesh, Decimate, Dyntopo, auto-fill holes, replace mesh data, triangulate, join/separate, or batch-clean a bound production mesh unless the current task explicitly authorizes that exact operation.
- Do not modify Rest Pose, bones, animation, Armature modifiers, vertex groups, shape keys, UVs, material-slot order, parenting, pivots, or accepted head/chest/full-body proportions in a modeling-only task.
- Subdivision, Bevel, and Weighted Normal are optional per-object tools. Numeric examples for widths, levels, thresholds, voxel size, bake distance, and polygon budgets are not MineLearning defaults.

Read-only inspection of the connected Blender 5.1.2 on 2026-09-06 confirmed:

- Boolean `solver`: `FLOAT`, `EXACT`, `MANIFOLD`; upstream `FAST` examples are invalid here.
- `bmesh.ops.boolean` is absent; use a verified Boolean modifier/operator with explicit objects and context.
- `WeightedNormalModifier.face_influence` is absent.
- Bevel limit methods `NONE`, `ANGLE`, `WEIGHT`, `VGROUP` exist.

Re-check before future execution because runtime and bundled docs can change.

## Overview

This skill provides expert guidance for Blender 5.x modeling and modifiers: configuring the modifier stack, using bmesh for procedural mesh creation/editing, running mesh operators, setting up sculpting workflows, and common modeling patterns (hard surface, retopology, booleans). The reference files contain the full modifier catalog and Python API patterns.

## MCP-First Approach

Prefer the project's existing Blender MCP for authorized modeling operations. If it is not connected or the live file cannot be confirmed, stop writes and report.

**Detection:** tool names alone do not prove a live connection or correct file. Perform a read-only version/path check.

**Tools used by this skill:**

- `execute_blender_code` — run `bpy` Python in the live session (primary action)
- `get_objects_summary`, `get_object_detail_summary` — inspect scene state before mutating
- `get_blendfile_summary_datablocks` / `_missing_files` / `_of_linked_libraries` / `_path_info` / `_usage_guess` — inventory the file
- `search_api_docs`, `get_python_api_docs`, `search_manual_docs` — confirm correct API/operator names before calling them
- `get_screenshot_of_window_as_image` / `_as_json`, `get_screenshot_of_area_as_image` — verify visual results
- `jump_to_tab_by_name`, `jump_to_tab_by_space_type`, `jump_to_view3d_object_by_name`, `jump_to_view3d_object_data_by_name` — drive the UI to make changes visible
- `render_thumbnail_to_path`, `render_viewport_to_path` — capture quick visual feedback

**Workflow:** inspect with a `get_*` / `search_*` tool → mutate via `execute_blender_code` → verify with a screenshot or summary tool. Keep code blocks small and idempotent so failures are easy to localize.

Keep the current project MCP configuration; this selected skill directory intentionally excludes upstream setup files and plugin configuration.

## Task Decision Tree

- **"Add a modifier for X"** -> Consult `references/modifier_reference.md` for the correct modifier type and properties
- **"Create mesh geometry procedurally"** -> See bmesh Workflow below and `references/python_api.md`
- **"Boolean operation between objects"** -> See Boolean Workflow
- **"Set up hard surface modeling"** -> See Hard Surface Pattern
- **"Retopologize a mesh"** -> See Retopology Workflow
- **"Clean up mesh"** -> See Mesh Cleanup
- **"Set up sculpting"** -> See Sculpt Setup
- **"Apply all modifiers"** -> See Modifier API in `references/python_api.md`

## Blender 5.1 Changes

### Boolean Modifier Speed Improvements
1. Boolean modifier operations are significantly faster in 5.1
2. The connected 5.1.2 exposes `FLOAT`, `EXACT`, and `MANIFOLD`; choose only after an RNA check and geometry test
3. Complex boolean operations on dense meshes see the largest speedups

### Corrective Flip Normals
1. New operator: `bpy.ops.mesh.corrective_flip_normals()`
2. Computes the correct normal orientation based on surrounding geometry context
3. More reliable than simple `normals_make_consistent` for non-manifold meshes

### Snap to Face Center
1. New snap option for snapping to the center of faces
2. Python: set `bpy.context.tool_settings.snap_elements` to include `'FACE_CENTER'`

### Loop Selection Delimiter
1. Improved edge loop selection now respects delimiter angle settings
2. Better control over which edges are considered part of a loop
## Modifier Stack Principles

1. **Order matters**: Modifiers are applied top-to-bottom. Mirror before Subdivision is different from Subdivision before Mirror.
2. **Viewport vs Render**: Many modifiers have separate viewport and render settings (e.g., `levels_viewport` vs `render_levels` on Subdivision Surface).
3. **Apply with caution**: Applying modifiers is destructive. Keep them live when possible.
4. **Non-destructive workflow**: Use modifier stack for iterative modeling. Apply only when needed for export or further editing.

### Common Modifier Stacking Order

1. **Mirror** (if symmetric)
2. **Array** (if repeating)
3. **Boolean** (cut/join operations)
4. **Solidify** (add thickness)
5. **Bevel** (edge bevels)
6. **Optional shading treatment** when the selected object needs it
7. **Optional Subdivision Surface** only when the form/topology requires it

## Boolean Workflow

### Non-Destructive Booleans (Modifier)

```python
import bpy

obj = bpy.context.active_object
cutter = bpy.data.objects["Cutter"]

mod = obj.modifiers.new(name="Boolean", type='BOOLEAN')
mod.operation = 'DIFFERENCE'  # DIFFERENCE, UNION, INTERSECT
mod.object = cutter
mod.solver = 'FLOAT'  # 5.1.2 inspected enum: FLOAT, EXACT, MANIFOLD
# Choose from current RNA and validate the actual result

# Hide cutter in viewport
cutter.hide_viewport = True
cutter.display_type = 'WIRE'
```

### Exact Solver Options

```python
mod.solver = 'EXACT'
mod.use_self = False          # Self-intersection handling
mod.use_hole_tolerant = True  # Better handling of non-manifold meshes
```

## Hard Surface Pattern

1. Start with base shape (Cube, Cylinder, etc.)
2. Add **Mirror** modifier for symmetry
3. Model with limited geometry — define major forms
4. Add supporting edge loops near edges to control subdivision
5. Add a selective **Bevel** only when the reference needs an edge radius
6. Add **Weighted Normal** only if inspected shading benefits
7. Add **Subdivision Surface** only when required by the selected surface and topology

```python
# Hard surface modifier stack
obj = bpy.context.active_object

# Mirror
mirror = obj.modifiers.new("Mirror", 'MIRROR')
mirror.use_axis = (True, False, False)
mirror.use_clip = True

# Bevel
bevel = obj.modifiers.new("Bevel", 'BEVEL')
bevel.limit_method = 'WEIGHT'
bevel.width = 0.02
bevel.segments = 3
bevel.profile = 0.5

# Optional Weighted Normal
wnormal = obj.modifiers.new("WeightedNormal", 'WEIGHTED_NORMAL')
wnormal.mode = 'FACE_AREA'
wnormal.keep_sharp = True

# Optional Subdivision Surface; example levels are not project defaults
subsurf = obj.modifiers.new("Subdivision", 'SUBSURF')
subsurf.levels = 2
subsurf.render_levels = 3
```

## Retopology Workflow

1. Import high-poly scan/sculpt
2. Add **Shrinkwrap** modifier to a new low-poly mesh
3. Set Shrinkwrap to `NEAREST_SURFACE` or `PROJECT`
4. Model low-poly mesh in edit mode — it snaps to high-poly surface
5. Remesh is destructive to topology and prohibited on bound production meshes; use only on an explicitly authorized isolated copy

```python
# Remesh (automated)
mod = obj.modifiers.new("Remesh", 'REMESH')
mod.mode = 'VOXEL'        # VOXEL, BLOCKS, SMOOTH, SHARP
mod.voxel_size = 0.05     # Smaller = more detail
mod.use_smooth_shade = True

# Shrinkwrap (manual retopology helper)
mod = lowpoly.modifiers.new("Shrinkwrap", 'SHRINKWRAP')
mod.target = highpoly
mod.wrap_method = 'NEAREST_SURFACEPOINT'
mod.wrap_mode = 'ON_SURFACE'
```

## Mesh Cleanup

### Common Cleanup Operations — isolated, explicitly authorized targets only

```python
import bpy

obj = bpy.context.active_object
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')

# Remove doubles (merge by distance)
bpy.ops.mesh.remove_doubles(threshold=0.0001)

# Recalculate normals
bpy.ops.mesh.normals_make_consistent(inside=False)

# Delete loose vertices/edges
bpy.ops.mesh.select_all(action='DESELECT')
bpy.ops.mesh.select_loose()
bpy.ops.mesh.delete(type='VERT')

# Fill holes only after proving the opening is unintended
bpy.ops.mesh.select_non_manifold()
bpy.ops.mesh.fill()

# Triangulate (for export)
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.quads_convert_to_tris()

bpy.ops.object.mode_set(mode='OBJECT')
```

### Decimate for Optimization — never automatic on production or bound meshes

```python
mod = obj.modifiers.new("Decimate", 'DECIMATE')
mod.decimate_type = 'COLLAPSE'  # COLLAPSE, UNSUBDIV, DISSOLVE
mod.ratio = 0.5                 # 50% of original faces
# mod.use_symmetry = True
# mod.symmetry_axis = 'X'
```

## Sculpt Setup

```python
import bpy

obj = bpy.context.active_object

# Add multires only to an authorized isolated object; it changes the modifier/data workflow
multires = obj.modifiers.new("Multires", 'MULTIRES')
for _ in range(4):
    bpy.ops.object.multires_subdivide(modifier="Multires", mode='CATMULL_CLARK')

# Or use Dyntopo (dynamic topology) — requires sculpt mode
bpy.ops.object.mode_set(mode='SCULPT')
bpy.context.sculpt_object.use_dynamic_topology_sculpting = True

# Sculpt settings
sculpt = bpy.context.tool_settings.sculpt
sculpt.detail_size = 12         # Dyntopo detail (lower = more detail)
sculpt.detail_refine_method = 'RELATIVE'  # RELATIVE, CONSTANT, BRUSH
sculpt.detail_type_method = 'RELATIVE'    # RELATIVE, CONSTANT, MANUAL
sculpt.use_smooth_shading = True
sculpt.symmetrize_direction = 'NEGATIVE_X'

# Brush settings
brush = sculpt.brush
brush.strength = 0.5
brush.size = 50
brush.auto_smooth_factor = 0.0
```

## bmesh Workflow

bmesh provides direct, efficient mesh creation and manipulation without `bpy.ops`:

### Create Mesh from Scratch

```python
import bpy
import bmesh

# Create empty mesh
mesh = bpy.data.meshes.new("ProcMesh")
bm = bmesh.new()

# Create geometry
v1 = bm.verts.new((0, 0, 0))
v2 = bm.verts.new((1, 0, 0))
v3 = bm.verts.new((1, 1, 0))
v4 = bm.verts.new((0, 1, 0))
bm.faces.new((v1, v2, v3, v4))

# Write to mesh and create object
bm.to_mesh(mesh)
bm.free()

obj = bpy.data.objects.new("ProcObj", mesh)
bpy.context.collection.objects.link(obj)
```

### Edit Existing Mesh

```python
import bmesh

obj = bpy.context.active_object
bm = bmesh.new()
bm.from_mesh(obj.data)

# Operations
bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=1)
bmesh.ops.translate(bm, vec=(0, 0, 1), verts=bm.verts[:])

# Write back
bm.to_mesh(obj.data)
bm.free()
obj.data.update()
```

For complete bmesh patterns and all modifier API details, see `references/python_api.md`.

## Debugging & Optimization

### Common Issues

- **Normals appear inverted**: Recalculate normals (`Mesh > Normals > Recalculate Outside`)
- **Shading artifacts on curved surfaces**: inspect topology, normals, bevel, and smooth-by-angle; Weighted Normal is optional
- **Boolean fails**: Ensure meshes are manifold (watertight). Try the Exact solver. Check for coplanar faces.
- **Modifier not applying**: Some modifiers require the object to be in Object mode. Check for dependencies.
- **Subdivision too dense**: Reduce viewport levels. Use adaptive subdivision for Cycles render.
- **Sculpt performance**: Lower multires preview level. Disable symmetry if not needed. Use Dyntopo constant detail.

### Performance Tips

1. Keep modifier stack short — each modifier is computed per depsgraph evaluation
2. Choose among current `FLOAT`, `EXACT`, and `MANIFOLD` only after a geometry test
3. Do not apply modifiers on animated/bound production objects without explicit authorization and rollback
4. Use instancing (Collection Instance) instead of duplicating high-poly objects
5. Treat Decimate as a scoped optimization decision, not a default export step

## Modifier Reference

For the complete catalog of all ~50 modifiers with type strings, properties, and use cases, consult `references/modifier_reference.md`.

Key categories:
- Generate: grep for "Array" or "Boolean" or "Bevel" or "Mirror" or "Solidify" or "Subdivision"
- Deform: grep for "Lattice" or "Shrinkwrap" or "Simple Deform" or "Curve"
- Physics: grep for "Cloth" or "Fluid" or "Particle" (see also blender-physics-simulation skill)

## Python API Reference

For complete Python API patterns including modifier management, bmesh operations, mesh operators, and sculpt configuration, consult `references/python_api.md`.

## Source and Adaptation

Adapted from [ra100/blender-claude-plugin blender-modeling-modifiers](https://github.com/ra100/blender-claude-plugin/tree/78e9151fdc9e01ce37f1d16a9b677c3047411885/skills/blender-modeling-modifiers), upstream plugin version 1.3.0, MIT. MineLearning corrections preserve the current MCP, fix inspected 5.1.2 API differences, and add production model/rig protection gates.
