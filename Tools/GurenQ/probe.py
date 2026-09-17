import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren'
d={}
paths=u.EditorAssetLibrary.list_assets(R+'/Animations',recursive=True)
opts=u.AnimPoseEvaluationOptions();opts.extract_root_motion=False
for p in paths:
 if any(s in p for s in ['Idle','NormalAttack']) and 'AM_' not in p:
  a=u.load_asset(p)
  if isinstance(a,u.AnimSequence):
   po=u.AnimPoseExtensions.get_anim_pose_at_time(a,0,opts)
   d[p]={n:str(u.AnimPoseExtensions.get_bone_pose(po,n,u.AnimPoseSpaces.WORLD)) for n in ['root','head','radiant_hand_r']}
for cls in ['AnimMontage','AnimSegment','SlotAnimationTrack','AnimationLibrary','AnimationBlueprintLibrary','AnimNotify_PlayMontageNotify','AnimNotifyEvent','AnimDataController','Skeleton','SkeletalMeshSocket']:
 c=getattr(u,cls,None);d[cls]=str(c.__doc__) if c else None
 if c:d[cls+'_methods']=[n for n in dir(c) if any(s in n for s in ['montage','notify','socket','slot','track','bone','controller'])]
a=u.load_asset(R+'/Animations/AM_Guren_NormalAttackCombo_FX');d['montage_tracks']=str(a.get_editor_property('slot_anim_tracks'))
bp=u.load_asset(R+'/Blueprints/BP_GurenRetargetTest');g=B.list_graphs(bp)[0]
d['bp_methods']=[n for n in dir(B) if any(s in n for s in ['pin','property','event','delegate'])]
(O/'probe.json').write_text(json.dumps(d,indent=2),encoding='utf8')
print('Q_PROBE_DONE')
