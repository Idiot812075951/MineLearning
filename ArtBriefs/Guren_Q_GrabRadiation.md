# Q Grab radiation preview — 2026-09-12

User authorizes extending their live edited Q_GrabLift frames 33–43 to approximately three seconds and adding simple red radiation and dummy opacity dissolve. This supersedes the preceding Q_GrabLift brief's exclusion of these effects. Blender preview only; no UE changes or export.

Source of truth: dirty live R14 saved intact to Q_GrabLift/Radiation/00_UserLive.blend. Work in V027A_R15_QGrabRadiation.blend. Maintain Q_GrabLift Action name and user key values. Map frames greater than 33 by 33 + (frame - 33) * 9. Preserve frames 1–33, unrelated Actions, production skeleton/rest/skin/meshes/materials, existing constraints and controls. Synchronize right IK and dummy action timing. New effects and dummy materials are non-export preview assets.

Radiation starts at 33, builds during the hold, then the dummy glows red and gradually becomes transparent by the end. Red palm rings and local lighting communicate the source. Keep the grab visible; avoid covering the entire character with effects. Deliver editable blend and MP4 only. Validate retiming against user snapshot, sampled motion, actual playback, and rendered effect stages.
