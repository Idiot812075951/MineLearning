"""Run in the UE editor Python console. Only writes the dedicated prototype folder."""
import unreal as u
import json
from pathlib import Path
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.actor import ActorTools as AT

ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
OUT = Path(u.Paths.project_dir()) / 'Saved/RadiantDissolve'
OUT.mkdir(parents=True, exist_ok=True)
E = u.EditorAssetLibrary
M = u.MaterialEditingLibrary
E.make_directory(ROOT)

def asset(name, cls, factory):
    return E.load_asset(ROOT+'/'+name) or u.AssetToolsHelpers.get_asset_tools().create_asset(name,ROOT,cls,factory)

def expr(mat, cls, x, y, **props):
    n=M.create_material_expression(mat,cls,x,y)
    for k,v in props.items(): n.set_editor_property(k,v)
    return n

def scalar(mat,name,value,x,y):
    return expr(mat,u.MaterialExpressionScalarParameter,x,y,parameter_name=name,default_value=value,group='Radiant Dissolve')

def wire(a,b,pin,out=''):
    assert M.connect_material_expressions(a,out,b,pin), (a,b,pin)

def custom_input(name):
    v=u.CustomInput(); v.set_editor_property('input_name',name); return v

def material():
    mat=asset('M_RadiantDissolve_Test',u.Material,u.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED)
    mat.set_editor_property('two_sided',True)
    wp=expr(mat,u.MaterialExpressionWorldPosition,-1200,0)
    origin=expr(mat,u.MaterialExpressionVectorParameter,-1200,180,parameter_name='DissolveOriginWS',default_value=u.LinearColor(-55,-35,70,0),group='Radiant Dissolve')
    noise=expr(mat,u.MaterialExpressionCustom,-900,0,description='Shared world-space front: broad distortion + fine breakup',output_type=u.CustomMaterialOutputType.CMOT_FLOAT1)
    noise.set_editor_property('inputs',[custom_input('P'),custom_input('Origin'),custom_input('Strength')])
    noise.set_editor_property('code','float3 q=P*0.075; float n=sin(q.x+sin(q.z*1.31))*sin(q.y*1.17-q.z*.43); float f=sin(q.x*3.7+q.z*2.1)*sin(q.y*3.1-q.x); return length(P-Origin)+Strength*(n*.72+f*.28);')
    wire(wp,noise,'P');wire(origin,noise,'Origin');wire(scalar(mat,'NoiseStrength',9,-1200,370),noise,'Strength')
    heat=scalar(mat,'HeatRadius',-40,-700,300); dissolve=scalar(mat,'DissolveRadius',-40,-700,400)
    width=scalar(mat,'DissolveEdgeWidth',4,-700,500); intensity=scalar(mat,'HeatIntensity',9,-700,600)
    masks=expr(mat,u.MaterialExpressionCustom,-420,0,description='Heat leads erosion. RGB = heat, hot rim, remaining mesh',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    masks.set_editor_property('inputs',[custom_input(s) for s in ['D','Heat','Dissolve','Width']])
    masks.set_editor_property('code','float h=smoothstep(0,22,Heat-D); float edge=(1-smoothstep(0,max(Width,.1),D-Dissolve))*step(Dissolve,D)*step(0,Dissolve); return float3(h,edge,step(Dissolve,D));')
    for n,p in [(noise,'D'),(heat,'Heat'),(dissolve,'Dissolve'),(width,'Width')]:wire(n,masks,p)
    tex=expr(mat,u.MaterialExpressionTextureSampleParameter2D,-400,-400,parameter_name='OreBaseColor',texture=E.load_asset('/Game/MineLearning/Mining/Ores/Iron/Textures/T_Ore_Iron_100_BaseColor'))
    surface=expr(mat,u.MaterialExpressionCustom,0,-180,description='Textured mineral -> red heat -> pink-white rim',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    surface.set_editor_property('inputs',[custom_input(s) for s in ['Base','Masks']])
    surface.set_editor_property('code','return lerp(Base,float3(.28,.003,.008),Masks.x*.88);')
    wire(tex,surface,'Base','RGB');wire(masks,surface,'Masks')
    emission=expr(mat,u.MaterialExpressionCustom,0,150,description='Controlled heat and narrow high-energy boundary',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
    emission.set_editor_property('inputs',[custom_input(s) for s in ['Masks','Intensity']])
    emission.set_editor_property('code','return Intensity*(pow(Masks.x,1.7)*float3(1,.006,.016)*.45 + pow(Masks.y,1.8)*float3(1,.21,.32)*2.3);')
    wire(masks,emission,'Masks');wire(intensity,emission,'Intensity')
    mask=expr(mat,u.MaterialExpressionComponentMask,0,440,r=False,g=False,b=True,a=False);wire(masks,mask,'')
    for n,p in [(surface,u.MaterialProperty.MP_BASE_COLOR),(emission,u.MaterialProperty.MP_EMISSIVE_COLOR),(mask,u.MaterialProperty.MP_OPACITY_MASK),(scalar(mat,'Roughness',.72,0,600),u.MaterialProperty.MP_ROUGHNESS)]:M.connect_material_property(n,'',p)
    M.recompile_material(mat);E.save_loaded_asset(mat)
    mi=asset('MI_RadiantDissolve_Test',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    M.set_material_instance_parent(mi,mat);E.save_loaded_asset(mi)
    return mi

def target_and_blueprint(mi):
    mesh=E.load_asset(ROOT+'/SM_RadiantDissolve_Ore_Test') or E.duplicate_asset('/Game/MineLearning/Mining/Ores/Iron/Meshes/SM_Ore_Iron_100',ROOT+'/SM_RadiantDissolve_Ore_Test')
    mesh.set_editor_property('allow_cpu_access',True)
    ns=mesh.get_editor_property('nanite_settings');ns.enabled=False;mesh.set_editor_property('nanite_settings',ns)
    E.save_loaded_asset(mesh)
    bp=E.load_asset(ROOT+'/BP_RadiantDissolve_Test') or BP.create(ROOT,'BP_RadiantDissolve_Test',u.Actor.static_class())
    comps=AT.get_components(BP.get_default_object(bp))
    def component(name,cls):
        return next((c for c in comps if c.get_name()==name+'_GEN_VARIABLE'),None) or AT.add_component(bp,cls.static_class(),name)
    target=component('TargetMesh',u.StaticMeshComponent);target.set_static_mesh(mesh);target.set_material(0,mi);target.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    origin=component('DissolveOrigin',u.SceneComponent);origin.set_editor_property('relative_location',u.Vector(-55,-35,70))
    fx=component('DissolveParticles',u.NiagaraComponent);fx.set_asset(E.load_asset(ROOT+'/NS_RadiantDissolve_Test'));fx.set_auto_activate(False)
    for name,typ,default in [('Elapsed','float',0),('Duration','float',1.8),('PropagationDistance','float',200),('NoiseStrength','float',9),('HeatIntensity','float',9),('DissolveEdgeWidth','float',4),('PreviewTime','float',0)]:
        if name not in BP.list_variables(bp): BP.add_variable(bp,name,typ)
        BP.set_variable_instance_editable(bp,name,name!='Elapsed')
    if 'DynamicMaterial' not in BP.list_variables(bp):BP.add_object_variable(bp,'DynamicMaterial',u.MaterialInstanceDynamic.static_class())
    BP.compile_blueprint(bp)
    cdo=BP.get_default_object(bp)
    for name,val in [('Duration',1.8),('PropagationDistance',200.),('NoiseStrength',9.),('HeatIntensity',9.),('DissolveEdgeWidth',4.)]:cdo.set_editor_property(name,val)
    for name in ['Initialize','TestDissolve','Reset','UpdateDissolve','ApplyFrame']:
        BP.add_function_graph(bp,name)
    BP.compile_blueprint(bp);E.save_loaded_asset(bp)
    graph=BP.get_graph(bp,'EventGraph')
    queries=['CreateDynamicMaterial','SetScalarParameter','SetVectorParameter','SetNiagaraVariable','GetWorldLocation','SetTimer','ClearTimer','GetPlayerController','EnableInput','SetVisibility','Activate','Deactivate','GetGameTime','Keyboard','GetTargetMesh','GetDissolveOrigin','GetDissolveParticles','GetDynamicMaterial','SetDynamicMaterial','Initialize','TestDissolve','ApplyFrame','Reset','UpdateDissolve','SetElapsed','GetElapsed','GetDuration','GetPropagationDistance','MakeLinearColor']
    data={q:BP.find_node_types(graph,q) for q in queries}
    (OUT/'nodes.json').write_text(json.dumps(data,default=str,indent=2))
    (OUT/'mesh.txt').write_text(str(mesh.get_bounding_box()))
    print('RADIANT_BASE_ASSETS_OK')

target_and_blueprint(material())



