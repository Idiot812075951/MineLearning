import unreal as u
from editor_toolset.toolsets.blueprint import BlueprintTools as BP
from editor_toolset.toolsets.actor import ActorTools as AT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
E=u.EditorAssetLibrary; M=u.MaterialEditingLibrary
mat=E.load_asset(ROOT+'/M_RadiantMote') if E.does_asset_exist(ROOT+'/M_RadiantMote') else u.AssetToolsHelpers.get_asset_tools().create_asset('M_RadiantMote',ROOT,u.Material,u.MaterialFactoryNew())
M.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('used_with_niagara_sprites',True)
uv=M.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-650,0)
pc=M.create_material_expression(mat,u.MaterialExpressionParticleColor,-650,220)
def custom(name,code,typ,inputs,x,y):
    n=M.create_material_expression(mat,u.MaterialExpressionCustom,x,y)
    n.set_editor_property('description',name);n.set_editor_property('code',code);n.set_editor_property('output_type',typ)
    ii=[]
    for name in inputs:
        i=u.CustomInput();i.set_editor_property('input_name',name);ii.append(i)
    n.set_editor_property('inputs',ii)
    return n
color=custom('Tiny white-hot cores / pink-red fringe','float r=length(UV-.5)*2; return lerp(float3(1,.015,.07),float3(1,.65,.8),pow(saturate(1-r),6))*220;',u.CustomMaterialOutputType.CMOT_FLOAT3,['UV'],-300,0)
alpha=custom('Soft circular mote / lifetime fade','float r=length(UV-.5)*2; return pow(saturate(1-r*r),2)*Alpha;',u.CustomMaterialOutputType.CMOT_FLOAT1,['UV','Alpha'],-300,240)
M.connect_material_expressions(uv,'',color,'UV');M.connect_material_expressions(uv,'',alpha,'UV');M.connect_material_expressions(pc,'A',alpha,'Alpha')
M.connect_material_property(color,'',u.MaterialProperty.MP_EMISSIVE_COLOR);M.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY)
M.recompile_material(mat);E.save_loaded_asset(mat)
bp=E.load_asset(ROOT+'/BP_RadiantDissolve_Test');cdo=BP.get_default_object(bp)
for k,v in [('HeatIntensity',80.),('NoiseStrength',12.),('DissolveEdgeWidth',2.5),('PropagationDistance',135.)]:cdo.set_editor_property(k,v)
BP.compile_blueprint(bp);E.save_loaded_asset(bp)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.get_current_level().get_outer().get_path_name()==ROOT+'/L_RadiantDissolve_Test.L_RadiantDissolve_Test'
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='RadiantDissolve_Test':
        for k,v in [('HeatIntensity',80.),('NoiseStrength',12.),('DissolveEdgeWidth',2.5),('PropagationDistance',135.),('PreviewTime',0.)]:a.set_editor_property(k,v)
        a.call_method('Reset')
    if a.get_actor_label()=='DemoExposure':
        s=a.get_editor_property('settings');s.set_editor_property('auto_exposure_bias',3.);a.set_editor_property('settings',s)
level.save_current_level()
print('RADIANT_VISUALS_REFINED')
