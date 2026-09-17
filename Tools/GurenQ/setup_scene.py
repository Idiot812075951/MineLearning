import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
from editor_toolset.toolsets.actor import ActorTools as A
O=Path(u.Paths.project_saved_dir())/'GurenQ';R='/Game/MineLearning/Characters/Guren';E=u.EditorAssetLibrary
pb=E.load_asset(R+'/Blueprints/BPC_GurenQPresentation');fx=E.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test')
if 'DissolveFX' not in B.list_variables(pb):B.add_object_variable(pb,'DissolveFX',fx.generated_class())
B.compile_blueprint(pb)
g=B.get_graph(pb,'EventGraph');out={}
for q in ['Dissolve','RadiantReset','SetRadiantDuration','SetRadiantShowOriginMarker','SpawnActorfromClass','RadiationChanged','SetDissolveFX','GetDissolveFX']:
 ids=list(B.find_node_types(g,q,[]));out[q]={s:str(B.get_node_type_pins(g,s)) for s in ids if 'DissolveFX' in s or s.endswith('|Dissolve') or 'RadiantReset' in s or 'RadiantDuration' in s or 'OriginMarker' in s or 'SpawnActorfromClass' in s or 'RadiationChanged' in s}
# Append a full-body Q slot after the existing locomotion/attack lower-body blend.
ab=E.load_asset(R+'/Blueprints/ABP_GurenLocomotion');ag=B.get_graph(ab,'AnimGraph');nodes=B.find_nodes(ag,'');infos=B.get_node_infos(nodes)
root=next(i for i in infos if i.node.get_class().get_name()=='AnimGraphNode_Root')
qslot=next((i for i in infos if i.node.get_class().get_name()=='AnimGraphNode_Slot' and str(i.node.get_editor_property('node').get_editor_property('slot_name'))=='QFullBody'),None)
if not qslot:
 out['slot_types']=list(B.find_node_types(ag,'Slot',[]))
 # Reuse the known existing Slot node type with a different configured slot name.
 existing=next(i for i in infos if i.node.get_class().get_name()=='AnimGraphNode_Slot')
 slot=B.create_node(ag,existing.type_id,u.IntPoint(700,100));v=slot.get_editor_property('node');v.set_editor_property('slot_name','QFullBody');slot.set_editor_property('node',v)
 qslot=B.get_node_infos([slot])[0]
if True:
 si=qslot;ri=next(p for p in root.input_pins if p.connected_pins)
 old=ri.connected_pins[0];B.break_pins(old,ri.pin_id)
 B.connect_pins(old,next(p.pin_id for p in si.input_pins if 'Pose' in p.name or p.name=='Source'))
 B.connect_pins(si.output_pins[0].pin_id,ri.pin_id)
 B.compile_blueprint(ab);E.save_loaded_asset(ab)
# Add the presentation component to the existing player Blueprint.
player=E.load_asset(R+'/Blueprints/BP_GurenRetargetTest');components=A.get_components(B.get_default_object(player))
if not any(c.get_name().startswith('QPresentation') for c in components):A.add_component(player,pb.generated_class(),'QPresentation')
B.compile_blueprint(player);E.save_loaded_asset(player)
pg=B.get_graph(player,'EventGraph');out['attack_nodes']=str(B.get_node_infos(B.find_nodes(pg,'')))
# A mannequin prototype, with the root placed at its actual head center.
dummy=E.load_asset(R+'/Blueprints/BP_QGrabTestDummy') or B.create(R+'/Blueprints','BP_QGrabTestDummy',u.QGrabTestDummy.static_class())
dc=B.get_default_object(dummy);mesh=E.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple') or E.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny')
out['mesh']=str(mesh)
if mesh:
 body=dc.get_editor_property('body');body.set_skeletal_mesh_asset(mesh);body.set_editor_property('relative_scale3d',u.Vector(1.65,1.65,1.65));body.set_editor_property('relative_rotation',u.Rotator(yaw=90));body.set_editor_property('relative_location',u.Vector(0,0,-298))
 B.compile_blueprint(dummy);E.save_loaded_asset(dummy)
(O/'setup_scene.json').write_text(json.dumps(out,indent=2),encoding='utf8')
print('Q_SCENE_SETUP_DONE')
