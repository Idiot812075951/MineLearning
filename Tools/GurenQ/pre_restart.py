import unreal as u,json,shutil
from pathlib import Path
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren'
d={}
for cls,methods in [('AnimationLibrary',['add_animation_notify_event','add_animation_notify_state_event','get_bone_pose_for_time']),('AnimDataController',['set_bone_track_keys']),('AnimMontageFactory',[]),('AnimSequence',['get_controller']),('Quat',['inverse','inversed']),('Skeleton',[])]:
 c=getattr(u,cls,None);d[cls]=str(c.__doc__) if c else None
 for m in methods:d[cls+'.'+m]=str(getattr(c,m,None).__doc__)
a=u.load_asset(R+'/Animations/Q/AN_Guren_Q_Dash');co=a.get_editor_property('controller');d['controller_class']=str(co.get_class());d['set_keys']=str(co.set_bone_track_keys.__doc__)
sk=u.load_asset(R+'/Skeletal/SK_Guren_Skeleton');d['sk_methods']=[n for n in dir(sk) if 'bone' in n or 'socket' in n or 'slot' in n]
u.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
backup=O/'BeforeQ';backup.mkdir(exist_ok=True)
for path in ['Characters/Guren/Blueprints/BP_GurenRetargetTest.uasset','Characters/Guren/Blueprints/ABP_GurenLocomotion.uasset','Characters/Guren/Skeletal/SK_Guren_Skeleton.uasset','Maps/L_Guren_Retarget_Test.umap']:
 p=Path(u.Paths.project_content_dir())/'MineLearning'/path
 if p.exists():shutil.copy2(p,backup/p.name)
(O/'api.json').write_text(json.dumps(d,indent=2),encoding='utf8')
print('Q_RESTART_READY')
