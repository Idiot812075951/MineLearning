import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren';E=u.EditorAssetLibrary
pb=E.load_asset(R+'/Blueprints/BPC_GurenQPresentation');g=B.get_graph(pb,'EventGraph')
code='''(event QEffects|EventRadiationChanged (bActive Target Origin)
  (if bActive
    (bind fx (Game|SpawnActorfromClass "/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test.BP_RadiantDissolve_Test_C" "(Translation=(X=0,Y=0,Z=-10000),Scale3D=(X=1,Y=1,Z=1))"))
    (Variables|Default|SetDissolveFX fx)
    (Rendering|SetActorHiddenInGame fx true)
    (Collision|SetActorEnableCollision fx false)
    (Class|BPRadiantDissolveTest|SetRadiantShowOriginMarker false fx)
    (Class|BPRadiantDissolveTest|SetRadiantDuration 2.866667 fx)
    (Class|BPRadiantDissolveTest|Dissolve fx Target Origin)
    (else
      (bind fx (Variables|Default|GetDissolveFX))
      (if (Utilities|IsValid fx)
        (Class|BPRadiantDissolveTest|RadiantReset fx)
        (Actor|DestroyActor fx)))))'''
B.write_graph_dsl(g,code);B.compile_blueprint(pb);E.save_loaded_asset(pb)
(O/'QPresentation.dsl').write_text(B.read_graph_dsl(g),encoding='utf8')
player=E.load_asset(R+'/Blueprints/BP_GurenRetargetTest');pg=B.get_graph(player,'EventGraph')
out={'player_queries':{q:list(B.find_node_types(pg,q,[])) for q in ['GetQSkill','IsQActive']}}
out['player_nodes']=[{'name':i.node.get_name(),'type':i.type_id,'inputs':[(p.name,str(p.pin_id),str(p.connected_pins)) for p in i.input_pins],'outputs':[(p.name,str(p.pin_id),str(p.connected_pins)) for p in i.output_pins]} for i in B.get_node_infos(B.find_nodes(pg,'')) if i.node.get_class().get_name() in ['K2Node_InputKey','K2Node_CustomEvent']]
# Freeze an actual mannequin idle, with its head at the root grip anchor.
dummy=E.load_asset(R+'/Blueprints/BP_QGrabTestDummy');dc=B.get_default_object(dummy);body=dc.get_editor_property('body')
idle=E.load_asset('/Game/Characters/Mannequins/Animations/Quinn/MF_Idle')
body.set_animation_mode(u.AnimationMode.ANIMATION_SINGLE_NODE)
ad=body.get_editor_property('animation_data');ad.set_editor_property('anim_to_play',idle);ad.set_editor_property('saved_looping',False);ad.set_editor_property('saved_playing',False);ad.set_editor_property('saved_position',0);body.set_editor_property('animation_data',ad)
opt=u.AnimPoseEvaluationOptions();pose=u.AnimPoseExtensions.get_anim_pose_at_time(idle,0,opt);head=u.AnimPoseExtensions.get_bone_pose(pose,'head',u.AnimPoseSpaces.WORLD)
scale=298/(head.translation.z+9);q=u.Rotator(yaw=90).quaternion();center=q.rotate_vector((head.translation+u.Vector(0,0,9))*scale)
body.set_editor_property('relative_scale3d',u.Vector(scale,scale,scale));body.set_editor_property('relative_location',-center)
dc.get_editor_property('grab_stand_point').set_editor_property('relative_location',u.Vector(-165,-53,-298))
B.compile_blueprint(dummy);E.save_loaded_asset(dummy)
es=u.get_editor_subsystem(u.EditorActorSubsystem)
if not any(isinstance(a,u.QGrabTestDummy) for a in es.get_all_level_actors()):
 for label,loc in [('Q Target - Dash 350cm',u.Vector(-2150,0,298)),('Q Target - Near 165cm',u.Vector(-2500,1200,298)),('Q Target - Dash 480cm',u.Vector(-1700,-1400,298))]:
  a=es.spawn_actor_from_class(dummy.generated_class(),loc,u.Rotator());a.set_actor_label(label);a.set_folder_path('Q Skill Targets')
out['dummy_head_source']=str(head);out['dummy_scale']=scale
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(O/'bind_presentation.json').write_text(json.dumps(out,indent=2),encoding='utf8')
print('Q_PRESENTATION_BOUND')
