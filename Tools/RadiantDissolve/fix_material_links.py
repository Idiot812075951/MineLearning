import unreal as u
from editor_toolset.toolsets.material import MaterialTools as MT
ROOT='/Game/MineLearning/VFX/RadiantDissolve'
for name in ['M_RadiantDissolve_Test','M_RadiantDissolve_Surface']:
    m=u.load_asset(ROOT+'/'+name)
    nodes=MT.get_expressions(m)
    call=next(n for n in nodes if isinstance(n,u.MaterialExpressionMaterialFunctionCall))
    for n in nodes:
        if isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description').startswith(('Textured mineral','Controlled heat')):
            MT.connect_expressions(call,'Masks',n,'Masks')
        if isinstance(n,u.MaterialExpressionComponentMask):MT.connect_expressions(call,'Masks',n,'')
    if name.endswith('Surface'):
        surface=next(n for n in nodes if isinstance(n,u.MaterialExpressionCustom) and n.get_editor_property('description').startswith('Textured mineral'))
        color=next(n for n in nodes if isinstance(n,u.MaterialExpressionVectorParameter) and str(n.get_editor_property('parameter_name'))=='BaseColor')
        MT.connect_expressions(color,'RGB',surface,'Base')
    MT.delete_unused_expressions(m)
    MT.recompile(m)
    u.EditorAssetLibrary.save_loaded_asset(m)
    print('VERIFIED_LINKS',name,[(n.get_name(),str(MT.get_expression_inputs(m,n))) for n in MT.get_expressions(m) if isinstance(n,u.MaterialExpressionCustom)])
fn=u.load_asset(ROOT+'/MF_RadiantDissolve')
print('VERIFIED_FUNCTION',[(n.get_name(),str(MT.get_expression_inputs(fn,n))) for n in MT.get_expressions(fn) if isinstance(n,(u.MaterialExpressionCustom,u.MaterialExpressionFunctionOutput))])
