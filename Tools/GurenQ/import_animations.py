import unreal as u,json
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren';dest=R+'/Animations/Q';source=Path(u.Paths.project_dir())/'ArtSource/Characters/GurenSeitenHakkyoShiki/UEExport/QSkill'
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(world,'Interchange.FeatureFlags.Import.FBX 0')
sk=u.load_asset(R+'/Skeletal/SK_Guren_Skeleton');tasks=[]
for src,name in [('Q_Dash','AN_Guren_Q_Dash'),('Q_GrabLift','AN_Guren_Q_GrabHeadIK')]:
 opt=u.FbxImportUI();opt.set_editor_property('automated_import_should_detect_type',False);opt.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_ANIMATION);opt.set_editor_property('original_import_type',u.FBXImportType.FBXIT_SKELETAL_MESH);opt.set_editor_property('import_mesh',False);opt.set_editor_property('import_animations',True);opt.set_editor_property('import_materials',False);opt.set_editor_property('import_textures',False);opt.set_editor_property('skeleton',sk)
 data=opt.get_editor_property('anim_sequence_import_data');data.set_editor_property('use_default_sample_rate',False);data.set_editor_property('custom_sample_rate',30);data.set_editor_property('animation_length',u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
 task=u.AssetImportTask();task.filename=str(source/(src+'.fbx'));task.destination_path=dest;task.destination_name=name;task.automated=True;task.replace_existing=False;task.save=True;task.options=opt;tasks.append(task)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
report={}
for task in tasks:
 paths=list(task.imported_object_paths);report[task.destination_name]={'paths':paths}
 for p in paths:
  a=u.load_asset(p)
  if isinstance(a,u.AnimSequence):
   a.set_editor_property('enable_root_motion','Dash' in task.destination_name)
   a.set_editor_property('force_root_lock','Dash' not in task.destination_name)
   a.set_editor_property('root_motion_root_lock',u.RootMotionRootLock.ANIM_FIRST_FRAME)
   u.EditorAssetLibrary.save_loaded_asset(a)
   report[task.destination_name]['length']=a.get_play_length()
   report[task.destination_name]['skeleton']=a.get_editor_property('skeleton').get_path_name()
   report[task.destination_name]['methods']=[n for n in dir(a) if any(k in n for k in ['pose','notify','frame','controller'])]
   opts=u.AnimPoseEvaluationOptions();opts.set_editor_property('extract_root_motion',False)
   rows={}
   for time in [0,min(.2,a.get_play_length()),a.get_play_length()]:
    pose=u.AnimPoseExtensions.get_anim_pose_at_time(a,time,opts)
    rows[str(time)]={bn:str(u.AnimPoseExtensions.get_bone_pose(pose,bn,u.AnimPoseSpaces.WORLD)) for bn in ['root','head','radiant_hand_r','socket_grab_point']}
   report[task.destination_name]['poses']=rows
(O/'animation_import.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('GUREN_Q_IMPORT_DONE')
