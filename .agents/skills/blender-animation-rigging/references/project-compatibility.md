# MineLearning Compatibility and Audit Scope

Project adaptation: 2026-09-05. Source commit is recorded in SKILL.md. This skill supplements, not replaces, the current Asset Brief and `mining-game-3d-asset-production`.

## Existing Humanoid / Mecha Rig Work

- Resolve the current saved source and retain a rollback point before asset writes. Preserve unrelated unsaved Blender state. Stop writes if MCP or source-path verification fails.
- Baseline affected bone names/parents/Rest/axes/lengths, bindings, sockets, Action names/slots and NLA. Do not rebuild the production rig to follow a generic armature example.
- Guren's current Brief protects the production skeleton, wrist launch/rejoin chain, independent fingers and Wing Root hierarchy. Reuse the existing authoring control layer when authorized. Installing this skill does not itself authorize IK creation or edits.
- A detached wrist is an intentional mechanical state; shoulder, upper arm, elbow and the remaining coupler must stay continuous. Check transitions, not only detached endpoints. Do not hide disconnected geometry or offset bound meshes to disguise a bad chain.
- IK/FK needs an agreed control space, joint plane, target reach and no-pop matching. A pair of influence drivers alone does not implement a complete no-pop switch. Rigid segments must not stretch to reach an impossible target.
- Keep palm target position, palm normal, wrist roll and elbow pole as distinct checks. For legs, verify foot contact and support; for wings, verify root-driven hierarchy and clearance. Visual similarity still comes from the approved image/reference library, not these technical recipes.
- Scope FCurve edits to the intended Action slot, paths, channels and frame range. Preserve quaternion continuity and inspect Euler wrap where applicable. Never bulk-convert protected baked Actions to Bezier.
- When authorized to bake, use a recoverable destination, exact selected objects/bones and a bounded frame range. Preserve constraints/parents and original Actions unless their removal is explicitly requested. Re-evaluate the baked result independently of the authoring controls, including NLA state and slot assignment.
- Continuous playback and contact/impact/launch checks are required for motion claims. Numeric rig integrity does not prove an attractive pose or good animation. No UE export or UE-ready claim without the production skill's applicable checks.

## Verified Upstream Corrections

Read-only RNA inspection in the connected Blender **5.1.2** confirmed:

- The IK constraint enum is `IK`.
- `ActionSlot.handle` is a read-only integer; assign `AnimData.action_slot` to a compatible slot from the assigned Action instead.
- `bpy.ops.pose.apply_to_basis` is absent. Do not replace it with a destructive Rest-pose operation to repair a pose.
- FCurve modifier creation takes a `type`; Smooth exposes `sigma` and `filter_width`, not a Gaussian blend-type enum.
- Bone scale inheritance is `inherit_scale`; old `bone.layers` is absent. Use Bone Collections.
- B-Bone bend properties use X/Z (`bbone_curveinz`, `bbone_curveoutz`), not the upstream Y names.
- Child Of has `inverse_matrix` and `set_inverse_pending`, not a `set_inverse()` method. Correct inverse setup is context/space dependent: consult current official documentation and validate transform preservation; do not guess a matrix.
- `LOCAL_OWNER_ORIENT` appears in target-space RNA, not owner-space RNA. Available/meaningful spaces also depend on the constraint and target type.

Additional edits correct the NLA time-scale explanation, action-copy example, mode/selection preconditions, misleading completeness claims and the out-of-package setup link.

This is a scoped compatibility audit, **not execution testing of every upstream recipe**. Read-only live checks did not modify or save the asset. Re-check current Blender RNA/official API documentation before using a version-sensitive recipe; test risky changes on an isolated disposable rig before applying them to an approved asset copy.
