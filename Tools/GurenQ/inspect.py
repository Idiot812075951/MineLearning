import unreal as u, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
O=Path(u.Paths.project_saved_dir())/'GurenQ';O.mkdir(exist_ok=True)
R='/Game/MineLearning/Characters/Guren'
def props(o,names):
 d={}
 for n in names:
  try:d[n]=str(o.get_editor_property(n))
  except Exception as e:d[n]='ERROR '+str(e)
 return d
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
out={'world':world.get_path_name(),'engine':u.SystemLibrary.get_engine_version(),'project':u.Paths.project_dir()}
for name in ['BP_GurenRetargetTest','BP_GurenCharacter','ABP_GurenLocomotion']:
 bp=u.load_asset(R+'/Blueprints/'+name);cdo=u.get_default_object(bp.generated_class())
 out[name]={'class':cdo.get_class().get_path_name(),'properties':props(cdo,['root_motion_mode','target_skeleton']), 'variables':BP.list_variables(bp),'graphs':[(g.get_name(),g.get_path_name()) for g in BP.list_graphs(bp)]}
 if name.startswith('BP_'):
  out[name]['components']=[{'name':c.get_name(),'class':c.get_class().get_name(),'props':props(c,['relative_location','relative_rotation','relative_scale3d','skeletal_mesh_asset','anim_class','capsule_half_height','capsule_radius'])} for c in cdo.get_components_by_class(u.ActorComponent)]
 for g in BP.list_graphs(bp):
  if g.get_name() in ['EventGraph','AnimGraph']:
   (O/(name+'_'+g.get_name()+'.dsl')).write_text(BP.read_graph_dsl(g),encoding='utf-8')
   nodes=BP.find_nodes(g,'');(O/(name+'_'+g.get_name()+'_nodes.json')).write_text(str(BP.get_node_infos(nodes)),encoding='utf-8')
d=u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test');dc=u.get_default_object(d.generated_class())
out['dissolve']={'variables':props(dc,BP.list_variables(d)),'functions':str(BP.list_functions(d))}
for g in BP.list_graphs(d):
 if g.get_name() in ['Dissolve','RadiantReset','RadiantPlay','RadiantFinish','RadiantApplyFrame','RadiantInitialize','EventGraph']:(O/('Dissolve_'+g.get_name()+'.dsl')).write_text(BP.read_graph_dsl(g),encoding='utf-8')
out['level_actors']=[{'name':a.get_actor_label(),'class':a.get_class().get_path_name(),'location':str(a.get_actor_location())} for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()]
(O/'inspection.json').write_text(json.dumps(out,indent=2,default=str),encoding='utf-8')
print('GUREN_Q_INSPECTION_DONE')
