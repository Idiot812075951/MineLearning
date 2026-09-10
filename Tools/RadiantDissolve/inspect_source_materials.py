"""Inspect source and dissolve-adapter materials without mutating assets."""
import json
from pathlib import Path

import unreal as u


ROOT = '/Game/MineLearning/VFX/RadiantDissolve'
E = u.EditorAssetLibrary
M = u.MaterialEditingLibrary


def material_info(path):
    material = E.load_asset(path)
    if not material:
        return {'path': path, 'missing': True}
    info = {
        'path': material.get_path_name(),
        'class': material.get_class().get_name(),
        'base': material.get_base_material().get_path_name(),
    }
    if isinstance(material, u.Material):
        info['blend_mode'] = str(material.get_editor_property('blend_mode'))
        info['used_with_skeletal_mesh'] = material.get_editor_property(
            'used_with_skeletal_mesh'
        )
        info['base_color_input'] = None
        node = M.get_material_property_input_node(
            material, u.MaterialProperty.MP_BASE_COLOR
        )
        if node:
            info['base_color_input'] = {
                'name': node.get_name(),
                'class': node.get_class().get_name(),
                'output': M.get_material_property_input_node_output_name(
                    material, u.MaterialProperty.MP_BASE_COLOR
                ),
            }
        info['custom_nodes'] = [
            {
                'description': expression.get_editor_property('description'),
                'code': expression.get_editor_property('code'),
            }
            for expression in M.get_material_expressions(material)
            if isinstance(expression, u.MaterialExpressionCustom)
        ]
    return info


mesh_materials = {}
for mesh_path in [
    '/Game/MineLearning/Characters/Gunner/Meshes/SK_Gunner',
    '/Game/MineLearning/Characters/Gunner/Weapons/AK/Meshes/SM_AK_Body',
    '/Game/MineLearning/Characters/Gunner/Weapons/AK/Meshes/SM_AK_Magazine',
]:
    mesh = E.load_asset(mesh_path)
    rows = []
    property_name = 'materials' if isinstance(mesh, u.SkeletalMesh) else 'static_materials'
    for slot in mesh.get_editor_property(property_name):
        interface = slot.get_editor_property('material_interface')
        rows.append({
            'material': interface.get_path_name() if interface else None,
            'base': (
                interface.get_base_material().get_path_name() if interface else None
            ),
        })
    mesh_materials[mesh_path] = rows


paths = [
    '/Game/MineLearning/Characters/Gunner/Materials/M_Gunner',
    ROOT + '/M_RadiantDissolve_GunnerSurface',
    '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Metal',
    '/Game/MineLearning/Characters/Gunner/Weapons/AK/Materials/M_AK_Polymer',
    ROOT + '/M_RadiantDissolve_AKMetalSurface',
    ROOT + '/M_RadiantDissolve_AKPolymerSurface',
    ROOT + '/M_RadiantDissolve_Surface',
]
payload = {
    'mesh_materials': mesh_materials,
    'materials': [material_info(path) for path in paths],
}
output = Path(u.Paths.project_saved_dir()) / 'RadiantDissolve' / 'source_materials.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(payload, indent=2), encoding='utf-8')
print('RADIANT_SOURCE_MATERIALS_DUMPED', output)
