import unreal as u,json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as B
R='/Game/MineLearning/Characters/Guren';O=Path(u.Paths.project_saved_dir())/'GurenQ';E=u.EditorAssetLibrary
pb=E.load_asset(R+'/Blueprints/BPC_GurenQPresentation');g=B.get_graph(pb,'EventGraph')
infos=B.get_node_infos(B.find_nodes(g,''));out={}
for i in infos:
 if 'SpawnActor' in i.type_id or 'IsValid' in i.type_id:out[i.type_id]={'node':i.node.get_path_name(),'in':[(p.name,p.type_id) for p in i.input_pins],'out':[(p.name,p.type_id) for p in i.output_pins]}
out['make']=list(B.find_node_types(g,'MakeTransform',[]))
out['validpins']=str(B.get_node_type_pins(g,'Utilities|IsValid'))
(O/'presentation_fix_probe.json').write_text(json.dumps(out,indent=2),encoding='utf8')
print('Q_FX_PROBE')
