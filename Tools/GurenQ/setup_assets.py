import unreal as u,json,math
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren';E=u.EditorAssetLibrary
manifest=json.loads((Path(u.Paths.project_dir())/'ArtSource/Characters/GurenSeitenHakkyoShiki/UEExport/QSkill/export_manifest.json').read_text())
opt=u.AnimPoseEvaluationOptions();opt.extract_root_motion=False
grab=u.load_asset(R+'/Animations/Q/AN_Guren_Q_GrabHeadIK');dash=u.load_asset(R+'/Animations/Q/AN_Guren_Q_Dash');sk=grab.get_editor_property('skeleton')
# Calibration uses the source FBX pose before applying the existing runtime facing convention.
if E.get_metadata_tag(grab,'GurenQFacing')!='RuntimeY':
 pose=u.AnimPoseExtensions.get_anim_pose_at_time(grab,.2,opt)
 hand=u.AnimPoseExtensions.get_bone_pose(pose,'radiant_hand_r',u.AnimPoseSpaces.WORLD)
 xyz=manifest['animations']['Q_GrabLift']['head_targets_blender_m']['7']
 grip=hand.inverse_transform_location(u.Vector(-xyz[1]*100,-xyz[0]*100,xyz[2]*100))
 (O/'grip.json').write_text(json.dumps([grip.x,grip.y,grip.z]))
 for anim in [dash,grab]:
  samples=[]
  for i in range(anim.get_editor_property('number_of_sampled_keys')):
   p=u.AnimPoseExtensions.get_anim_pose_at_time(anim,i/30,opt)
   samples.append({b:u.AnimPoseExtensions.get_bone_pose(p,b,u.AnimPoseSpaces.LOCAL) for b in ['root','pelvis']})
  yaw=u.Rotator(0,0,90).quaternion()
  # Rotator positional order is pitch/yaw/roll in Python? Use named fields to remove ambiguity.
  yaw=u.Rotator(pitch=0,yaw=90,roll=0).quaternion()
  roots=[];hips=[]
  for s in samples:
   rt=s['root'];pel=s['pelvis'];r=rt.rotation
   q=r.inversed()*yaw*r
   rt.translation=yaw.rotate_vector(rt.translation)
   pel.translation=q.rotate_vector(pel.translation);pel.rotation=q*pel.rotation
   roots.append(rt);hips.append(pel)
  co=anim.get_editor_property('controller');co.open_bracket('Q runtime facing')
  for bone,ts in [('root',roots),('pelvis',hips)]:
   assert co.set_bone_track_keys(bone,[t.translation for t in ts],[t.rotation for t in ts],[t.scale3d for t in ts])
  co.close_bracket()
  E.set_metadata_tag(anim,'GurenQFacing','RuntimeY');E.save_loaded_asset(anim)
grip=u.Vector(*json.loads((O/'grip.json').read_text()));u.GurenQAssetSetup.configure_skeleton(sk,grip);E.save_loaded_asset(sk)
for anim,name in [(dash,'AM_Q_Dash'),(grab,'AM_Q_GrabDissolve')]:
 path=R+'/Animations/Q/'+name
 montage=u.load_asset(path)
 if not montage:
  fac=u.AnimMontageFactory();fac.set_editor_property('target_skeleton',sk);fac.set_editor_property('source_animation',anim)
  montage=u.AssetToolsHelpers.get_asset_tools().create_asset(name,R+'/Animations/Q',u.AnimMontage,fac)
 tracks=montage.slot_anim_tracks;tracks[0].set_editor_property('slot_name','QFullBody');montage.set_editor_property('slot_anim_tracks',tracks)
 bi=montage.get_editor_property('blend_in');bi.set_editor_property('blend_time',.06 if anim==dash else .08);montage.set_editor_property('blend_in',bi)
 bo=montage.get_editor_property('blend_out');bo.set_editor_property('blend_time',.05 if anim==dash else .15);montage.set_editor_property('blend_out',bo)
 montage.set_editor_property('blend_out_trigger_time',0 if anim==dash else .10)
 if 'Q Events' in [str(x) for x in u.AnimationLibrary.get_animation_notify_track_names(montage)]:
  u.AnimationLibrary.remove_animation_notify_track(montage,'Q Events')
 if True:
  u.AnimationLibrary.add_animation_notify_track(montage,'Q Events',u.LinearColor(1,.15,.1,1))
  pairs=[(.55,'DashArrival')] if anim==dash else [(.2,'GrabContact'),(32/30,'StartDissolve'),(118/30,'DissolveFinish'),(121/30,'SkillEnd')]
  for time,event in pairs:
   notify=u.AnimationLibrary.add_animation_notify_event(montage,'Q Events',time,u.AnimNotify_GurenQEvent);notify.set_editor_property('event',event)
  if anim==dash:
   state=u.AnimationLibrary.add_animation_notify_state_event(montage,'Q Events',4/30,.55-4/30,u.AnimNotifyState_MotionWarping)
   modifier=u.new_object(u.RootMotionModifier_SkewWarp,outer=state)
   modifier.set_editor_property('warp_target_name','Q_DashTarget');modifier.set_editor_property('warp_translation',True);modifier.set_editor_property('ignore_z_axis',True);modifier.set_editor_property('warp_rotation',True)
   state.set_editor_property('root_motion_modifier',modifier)
 E.save_loaded_asset(montage)
# Create presentation Blueprint. Its event graph is filled by a separate small script.
bp=E.load_asset(R+'/Blueprints/BPC_GurenQPresentation') or B.create(R+'/Blueprints','BPC_GurenQPresentation',u.GurenQPresentationComponent.static_class())
c=u.get_default_object(bp.generated_class());c.set_editor_property('dash_montage',u.load_asset(R+'/Animations/Q/AM_Q_Dash'));c.set_editor_property('grab_montage',u.load_asset(R+'/Animations/Q/AM_Q_GrabDissolve'));E.save_loaded_asset(bp)
g=B.get_graph(bp,'EventGraph')
queries=['RadiationChanged','SpawnActor','GetDissolve','Dissolve','RadiantReset','SetRadiantDuration','GetOwner','DestroyActor','SetActorHiddenInGame','IsValid']
out={q:list(B.find_node_types(g,q,[])) for q in queries}
out['pose_methods']=[n for n in dir(u.AnimPoseExtensions) if 'bone' in n]
out['controller_methods']=[n for n in dir(grab.controller) if 'bone' in n]
out['dummy_meshes']=[p for p in E.list_assets('/Game/Characters/Mannequins',recursive=True) if any(s in p for s in ['/SKM_','/MF_Idle','/MM_Idle'])]
out['world']=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name()
(O/'setup_assets.json').write_text(json.dumps(out,indent=2),encoding='utf8')
print('Q_ASSETS_SETUP_DONE')
