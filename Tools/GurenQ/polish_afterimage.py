import unreal as u
from editor_toolset.toolsets.material import MaterialTools as MT
material = u.load_asset('/Game/MineLearning/Characters/Guren/VFX/M_QDashAfterimage')
for expression in MT.get_expressions(material):
    if isinstance(expression, u.MaterialExpressionVectorParameter) and str(expression.get_editor_property('parameter_name')) == 'TrailColor':
        expression.set_editor_property('default_value', u.LinearColor(.85, .008, .11, 1))
u.MaterialEditingLibrary.recompile_material(material)
u.EditorAssetLibrary.save_loaded_asset(material)
print('Q_AFTERIMAGE_POLISHED')
