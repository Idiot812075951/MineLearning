# Guren Q_Dash — Blender animation brief

Scope: only the dynamic dash preceding a future Q_GrabLift. No grab, lift, dissolve, gameplay changes or UE import. The user's two Q_Dash sheets guide intent, timing and silhouettes; joint integrity and the current model take priority.

Source: live V027A_R10_FlightControlWings.blend, SK_Guren_Current, Anim_CombatIdle at frame 0. Preserve the unsaved live state in Q_Dash/00_UserLive.blend before authoring. Work file: V027A_R11_QDash.blend. New Action exactly Q_Dash. Existing 35 Actions, 59-bone hierarchy/rest, meshes, bindings, materials, independent fingers, right arm launch mechanism and controller Actions are protected.

Identity: approved existing crimson/gold Guren and magenta energy wings; no modeling or palette changes. This is an animation addition to the existing approved character.

Timing: 30 fps, frames 1–18 (0.567 s between endpoints). Five main poses: Start 1, Anticipation 4, Launch 7, Dash 11, Arrival 18. Additional contact/braking breakdowns allowed. Root motion target 2.8 metres along Blender -Y on root bone, zero vertical root drift, constant Armature Object. Pelvis supplies compression and flight height. No stretch.

Start close to the current CombatIdle. Fast compression, rear leg push, asymmetric airborne stride and forward body/eye line. Wings sweep backward then brake open. Arrival settles in a planted stagger, right claw ready in front to connect to a later grab, without actually grabbing.

Use sparse FK keys; existing right arm IK disabled only through new Action's existing R5_ik_enabled property. Analytic contact solving may determine FK leg poses without adding bones/constraints. Review stepped blocking, smooth motion and intermediate frames, ground contacts, shoulder/wrist continuity, wing clearance and root speed. Preserve old Action curves byte-equivalently in a data fingerprint. MP4 preview only. UE export/warping is a later task, not claimed validated here.
