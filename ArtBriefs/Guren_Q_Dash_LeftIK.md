# Q_Dash left-arm IK revision

Current instruction authorizes adding a left wrist IK target and elbow pole matching the existing right-hand authoring system. Modify Q_Dash Launch F7 and Dash F11 plus necessary transitions: bent elbow, left closed fist, dorsal side world-up and thumb/fist-eye side inward (character -X). Use first reference's side silhouette as intent; maintain mechanical proportions and attachment continuity.

Source of truth: live V027A_R11_QDash.blend, currently dirty at F7, Q_Dash. Snapshot complete live state to Q_Dash/LeftIK/00_UserLive.blend, then work in V027A_R12_QDash_LeftIK.blend. Preserve existing root/pelvis/body/head/legs/right arm/wings animation and all other Actions, skeleton bone names/parents/rest, meshes/skin/materials. No UE or code changes.

New controls: CTRL_R12_LeftHand_IK (G: wrist, R: palm orientation), CTRL_R12_LeftElbow_Pole (G: elbow plane). Existing production bones are reused; no stretch. New Action-scoped enable property isolates constraints from old clips. New controller animation Actions hold the Dash control keys. Finger curl uses existing finger controls/drivers; no remodeling. No claim of automatic seamless IK/FK switching outside the matched Dash.

Verify controller movement and orientation independently, chain integrity/reach, correct closed fist and palm axes at F7/F11, continuity across the clip, unchanged protected channels and old Actions. Save new version and provide updated MP4.
