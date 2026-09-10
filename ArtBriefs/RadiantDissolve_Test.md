# Radiant Dissolve VFX Prototype

Status: incomplete. This is a reviewable VFX prototype, not a finished production dissolve feature.

Scope: standalone UE 5.8 visual prototype for ordinary Static Mesh and Skeletal Mesh components. The current user prompt is the visual acceptance brief.

Source of truth: saved UE assets in `/Game/MineLearning/VFX/RadiantDissolve`. `Tools/RadiantDissolve` contains supporting authoring and verification scripts, not the runtime implementation.

Visual identity: source material with a narrow transparent red contour first, then contact-point deep-red heating and irregular erosion, a held emissive remnant outline, then readable white pearl motes that disperse outward, shrink and fade. The result should feel like high-energy structural decomposition, not an explosion or vertical dissolve.

Runtime target: an independently placed controller exposes one `Dissolve(Target, OriginWS)` call, enumerates visible Mesh Components on the Actor and recursively attached Actors, handles every material slot, uses bounds-derived coverage, and restores all source materials and visibility on Reset. Mesh type, Niagara data interface, source-material compatibility and adapter selection are internal implementation details and are not user parameters.

Origin: configurable world-space `DissolveOriginWS`. Automatic placement uses the target Bounds center with a small positive Z offset; a visible marker is provided. Future palm contact will supply this coordinate.

Protected scope: no Guren character, grab logic, hit detection, damage, slow motion, camera shake, SFX, gameplay Actor destruction, OreBuddy/Gunner/ore business changes, execution framework, Blender edit, source mesh edit, rig edit, or animation edit.

Particle timing/direction: particle Spawn Rate remains zero until normalized phase 0.84, then uses one exposed rate. Velocity always has a positive component away from OriginWS; no inward random branch remains. One exposed speed controls both supported surface samplers.

Current limitations: no ISM/HISM per-instance targeting, Geometry Collection, Niagara/procedural mesh targets, or automatic runtime injection of dissolve code into an arbitrary compiled master material. Production master materials must include `MF_RadiantDissolve`; legacy demo materials use an internal compatibility layer.

Acceptance: Blueprint compiles with warnings treated as errors; internal Niagara samplers compile without errors/warnings; the same `Dissolve(Target, OriginWS)` call resolves both test target forms; phase 0.70 has Spawn Rate 0 and phase 0.90 has the configured rate/speed; Reset always begins intact regardless of travel/noise tuning.
