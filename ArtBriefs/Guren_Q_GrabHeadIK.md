# Grab head gaze — 2026-09-13

User requests an editable head IK control and Guren looking at the enemy's head while lifting it. Source is current live R15, including unsaved edits, backed up to Q_GrabLift/HeadLook/00_UserLive.blend. Work in V027A_R16_QGrabHeadIK.blend.

Implement directional look-at IK using a target control, stable upright aim frame and rotation constraint on head. No positional neck stretch or production skeleton additions. Maintain anatomical forward eye direction, use eye height compensation, fade gaze in during contact/lift and out as dummy disappears. Control follows dummy head and has editable local offset and influence. Gate effect to Q_GrabLift action; preserve old head keyframes underneath the constraint. Preserve all other bone motion, existing Actions/curves, root, mesh/material/skin and radiation preview. Validate targeting, temporal continuity, original animation isolation, views and playback. Blender and MP4 only.
