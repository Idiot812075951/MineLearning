# Radiant Dissolve — Final Scatter

Status: Current, 2026-09-15. Supersedes the held-outline visual design in RadiantDissolve_Test.md for this revision.

Scope: UE runtime VFX refinement requested by the user. One bright sweep covers the enemy surface, followed by dense small white motes from within its form, dispersing and fading until the enemy disappears. No held rim silhouette. UE assets under /Game/MineLearning/VFX/RadiantDissolve are the source of truth.

Identity: white energy decomposition is explicitly requested for Guren Q. Preserve Guren's approved red/gold identity and the level. Avoid explosions, large pearls, empty wireframes and a box-shaped particle cloud.

Protected: source meshes, Blender files, skeletons, animations, materials outside this VFX folder, Q target/facing/contact logic, montage notifies, damage/destruction timing, camera and HUD. Preserve Dissolve(Target, OriginWS), cancellation and exact source-material restoration.

Implementation: reuse the current GPU mesh samplers with inward normal offsets at randomized depths; this is a visually filled interior layer, not a watertight volume/SDF solver. Expose depth and useful normalized phase boundaries on the controller. Keep particle lifetimes inside the existing Q radiation window.

Acceptance: Blueprint and both Niagara systems compile; real Q shows an intact bright sweep, no persistent outline, readable dense white particles and a clean empty end. Verify reset/cancel and repeated execution. Save a baseline before editing, authoring scripts and actual UE captures.
