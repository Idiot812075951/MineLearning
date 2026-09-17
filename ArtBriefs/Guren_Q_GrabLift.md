# Q_GrabLift — current brief 2026-09-12

Create a new independent Q_GrabLift Action in Blender only, 30 FPS. User corrected the direction: target directly ahead and slightly toward Guren's anatomical RIGHT (-X in current Blender frame), never at the flank. This supersedes their earlier left-front typo and agrees with the work order. Disregard reference image 1 Pose 2; use side reference 2 for contact and lift mechanics. Keep right claw attack from the front, no reverse-hand or torso penetration.

Source: live saved V027A_R13_QDash_Smooth.blend, Q_Dash. Full snapshot Q_GrabLift/00_UserLive.blend; work in V027A_R14_QGrabLift.blend. Preserve all existing Actions, skeleton hierarchy/names/rest, production meshes/materials/skin, and existing IK controls. Start from evaluated Q_Dash F25, root-normalized at origin. Root stays fixed; pelvis/body provide weight shift.

Five stages: Grab Ready → fast Contact → Clamp with short stop → full-body diagonal Lift Start → stable high Lift Hold. Temporary grey human-proportion dummy permitted, separate non-export collection, head reliably follows actual claw contact after clamp. No execution/radiation/dissolve/death, UE, FX or animated camera.

Use sparse keys and appropriate breakdowns. Prefer existing two-bone IK arrangement for right arm with isolated action-scoped controls so old animations remain intact. Check claw/head surface clearance, arm/chest, dummy/wings, shoulder continuity, feet/ground, root stability and entry match. Review front, side, three-quarter and actual 30 FPS playback. Deliver .blend plus MP4, never GIF.
