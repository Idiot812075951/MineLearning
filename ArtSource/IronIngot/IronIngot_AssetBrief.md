# Iron Ingot Pickup — Current Asset Brief

## Basic information

- Asset: processed iron ingot pickup
- UE name: `SM_IronIngot`
- Category: resource / static pickup prop
- Intended UE folder: `/Game/MineLearning/Mining/Resources/IronIngot`
- Authoritative Blender file: `ArtSource/IronIngot/IronIngot.blend`
- Stage: compact production asset for the warehouse-processing loop

## Gameplay use and style

- The processor produces this item from one iron ore; the Hauler returns it to the Warehouse.
- It must remain readable on the processor belt, in the Hauler cargo box, on the ground, and in the warehouse.
- Target presentation size is the shared 30 cm maximum-dimension pickup standard.
- Form language: friendly stylized industrial metal, broad silhouette, rounded bevels, no sharp weapon-like edges.
- Identity/material: Tool Silver with a restrained dark gunmetal inset; no new faction color.

## Technical delivery

- Static Mesh, one combined export object, ground-centered pivot, Z-up.
- Authored dimensions: approximately 30 x 18 x 10.5 cm.
- Two stable material slots: `M_IronIngot_ToolSilver`, `M_IronIngot_Gunmetal`.
- Simple generated collision is sufficient; no skeleton, animation, sockets, or moving parts.
- Blender owns shape, material split, naming, normals, UVs, and scale. UE only owns runtime pickup placement and standardized visual sizing.

## Current task scope and protection

- Build only the processed iron-ingot pickup and import it into the intended UE folder.
- Do not modify `SellStation.blend`, existing ore/coin meshes, their UVs, or their materials.
- Preserve the existing processor, warehouse, carrier, and SellStation art assemblies.
- Stop after one validated production mesh; no variants, VFX, icon, animation, or decorative expansion.

## Acceptance priorities

1. Immediately reads as a processed metal ingot rather than a placeholder cube.
2. Correct 30 cm-class scale, ground pivot, clean normals/UVs, and stable material slots in UE.
3. Matches the project’s rounded, sturdy Q-style industrial sci-fi language at normal gameplay distance.
