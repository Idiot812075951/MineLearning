# Guren R17 — UE level visual polish

Status: Historical R17 delivery, superseded by `ArtBriefs/Guren_SurfaceFlow_R18.md` and `V027A_R18_SurfaceFlow.blend`. R17 is retained for rollback. User authorized implementation following `Docs/Guren_Visual_Diagnosis_20260914.md`. R17 evidence: `Docs/Guren_VisualPolish_R17_Delivery.md`.

Source: live `V027A_R16_QGrabHeadIK.blend`, including unsaved user changes. Preserve to `VisualPolish_R17/00_UserLive_R16.blend`; work in `V027A_R17_VisualPolish.blend`.

Goal: materially improve the model itself and deliver attractive appearance in the existing UE Guren test level. Lighting is supplementary. Preserve approved anime identity, proportions, red/orange-gold/green/cyan/silver/pink palette, head identity and mechanical silhouette. Official and animation references control form; product photography only supplements surface/assembly quality.

Authorized changes: chest green lens coverage and rim relationship, local chest surface continuity, targeted armor bevel/normal treatment after shoulder sample verification, coherent painted armor materials, energy membrane geometry and dedicated UV/masks, traceable UE material adaptation and source-normal import, modest test-level lighting/postprocess polish. The eight membrane meshes may be locally rebuilt as surface carriers with the same object names, pivots, bone parents and outline. Export staging may combine rigid mesh islands and triangulate for FBX while retaining editable source components.

Protected: all original bone names, hierarchy, Rest matrices, actions/keyframes, constraints/IK, object parents, pivots/transforms, armature modifiers and vertex groups; all non-target mesh geometry; original material-slot order. Lens UV modifications limited to that feature; new UV limited to membrane carriers. No animation replacement, retargeting, gameplay/UI rewrite or skeleton reference-pose update.

UE delivery: preserve existing `Skeletal/SK_Guren_Skeleton`, animations, Blueprint/AnimBP references and sockets. Back up affected assets/level before replacements. Import an isolated candidate first, verify skeleton/scale/material slots/normals, compare in level, then update the production mesh or its content while keeping existing references valid.

Review: front, three-quarter, side, back, chest close-up, wings spread/folded, idle/flight/attack/grab. Verify under neutral daylight and final level lighting; do not hide faults in darkness or bloom. Check lens green coverage, continuous highlights, meaningful armor creases, energy edges and transparent interiors, no doubled wing surfaces/sorting defects, no motion regression. Record actual exports, evidence, remaining limitations and protected-data comparison.
