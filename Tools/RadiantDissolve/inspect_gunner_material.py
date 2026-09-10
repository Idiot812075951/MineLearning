"""Record the Gunner source material inputs before authoring its VFX adapter."""
import json

import unreal as u
from editor_toolset.toolsets.material import MaterialTools as MT

material = u.load_asset('/Game/MineLearning/Characters/Gunner/Materials/M_Gunner')
rows = []
for node in MT.get_expressions(material):
    row = {'name': node.get_name(), 'class': node.get_class().get_name()}
    if isinstance(node, u.MaterialExpressionTextureBase):
        texture = node.get_editor_property('texture')
        row['texture'] = texture.get_path_name() if texture else None
    if isinstance(node, u.MaterialExpressionParameter):
        row['parameter_name'] = str(node.get_editor_property('parameter_name'))
    if isinstance(node, u.MaterialExpressionVectorParameter):
        value = node.get_editor_property('default_value')
        row['default'] = [value.r, value.g, value.b, value.a]
    if isinstance(node, u.MaterialExpressionScalarParameter):
        row['default'] = node.get_editor_property('default_value')
    if isinstance(node, u.MaterialExpressionCustom):
        row['description'] = node.get_editor_property('description')
        row['code'] = node.get_editor_property('code')
    rows.append(row)

report = {
    'material': material.get_path_name(),
    'blend_mode': str(material.get_editor_property('blend_mode')),
    'shading_model': str(material.get_editor_property('shading_model')),
    'two_sided': material.get_editor_property('two_sided'),
    'expressions': rows,
}
path = u.Paths.project_dir() + 'Saved/RadiantDissolve/gunner_material.json'
with open(path, 'w', encoding='utf-8') as handle:
    json.dump(report, handle, indent=2)
print('RADIANT_GUNNER_MATERIAL_INSPECTED')
