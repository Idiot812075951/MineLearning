import unreal as u, json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.material import MaterialTools as MT
bp=u.load_asset('/Game/MineLearning/VFX/RadiantDissolve/BP_RadiantDissolve_Test')
g=BP.get_graph(bp,'EventGraph')
queries=['GetComponentsByClass','CastToStaticMeshComponent','GetNumMaterials','GetMaterial','SetMaterial','GetComponentBounds','Distance(Vector)','VectorLength','SpawnSystemAttached','SetNiagaraStaticMeshComponent','DestroyComponent','IsValid','IsGameWorld','GetOwner','GetActorBounds','Array|Add','Array|Clear','Array|Get','GetScalarParameterValue','SetVisibility','Max','Min']
data={q:BP.find_node_types(g,q) for q in queries}
out=Path(u.Paths.project_dir())/'Saved/RadiantDissolve/reusable_nodes.json'
out.write_text(json.dumps(data,default=str,indent=2))
print('RADIANT_INSPECTION_READY')
types=['Niagara|SpawnSystemAttached','Niagara|SetNiagaraStaticMeshComponent','Collision|GetComponentBounds','Rendering|Material|GetScalarParameterValue','Utilities|Array|Add','Utilities|Array|Clear','Utilities|Array|Get(acopy)','Utilities|IsValid','Components|DestroyComponent','Actor|GetComponentsByClass']
pins={}
for t in types:
    try:
        n=BP.get_node_type_pins(g,t)
        pins[t]={'in':[(p.name,p.type_id) for p in n.input_pins],'out':[(p.name,p.type_id) for p in n.output_pins]}
    except Exception as error:pins[t]=str(error)
(out.parent/'reusable_pins.json').write_text(json.dumps(pins,indent=2))
